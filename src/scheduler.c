#include "reactor-uc/environment.h"
#include "reactor-uc/reactor-uc.h"
#include "reactor-uc/scheduler.h"

void Scheduler_register_for_cleanup(Scheduler *self, Trigger *trigger) {
  if (trigger->is_registered_for_cleanup) {
    return;
  }

  if (self->cleanup_ll_head) {
    self->cleanup_ll_tail->next = trigger;
    self->cleanup_ll_tail = trigger;
  } else {
    validaten(self->cleanup_ll_tail);
    self->cleanup_ll_head = trigger;
    self->cleanup_ll_tail = trigger;
  }
  trigger->is_registered_for_cleanup = true;
}

void Scheduler_prepare_timestep(Scheduler *self, tag_t tag) {
  self->env->current_tag = tag;
  self->executing_tag = true;
  self->reaction_queue.reset(&self->reaction_queue);
}

void Scheduler_clean_up_timestep(Scheduler *self) {
  assert(self->executing_tag);
  assert(self->reaction_queue.empty(&self->reaction_queue));
  self->executing_tag = false;
  Trigger *cleanup_trigger = self->cleanup_ll_head;

  while (cleanup_trigger) {
    Trigger *this = cleanup_trigger;
    assert(!(this->next == NULL && this != self->cleanup_ll_tail));
    this->cleanup(this);
    cleanup_trigger = this->next;
    this->next = NULL;
    this->is_registered_for_cleanup = false;
  }

  self->cleanup_ll_head = NULL;
  self->cleanup_ll_tail = NULL;
}

void Scheduler_run_timestep(Scheduler *self) {
  while (!self->reaction_queue.empty(&self->reaction_queue)) {
    Reaction *reaction = self->reaction_queue.pop(&self->reaction_queue);
    validate(reaction);
    reaction->body(reaction);
  }
}

void Scheduler_terminate(Scheduler *self) {
  Environment *env = self->env;
  printf("Scheduler terminating\n");
  self->prepare_timestep(self, env->stop_tag);

  if (env->has_async_events) {
    env->platform->leave_critical_section(env->platform);
  }

  Trigger *shutdown = &self->env->shutdown->super;
  while (shutdown) {
    shutdown->prepare(shutdown);
    shutdown = shutdown->next;
  }
  self->run_timestep(self);
  self->clean_up_timestep(self);
}

void Scheduler_handle_builtin(Trigger *trigger) {
  do {
    trigger->prepare(trigger);
    if (trigger->type == TRIG_STARTUP) {
      trigger = (Trigger *)((Startup *)trigger)->next;
    } else {
      trigger = (Trigger *)((Shutdown *)trigger)->next;
    }
  } while (trigger);
}

void Scheduler_pop_events(Scheduler *self, tag_t next_tag) {
  do {
    Event event = self->event_queue.pop(&self->event_queue);

    Trigger *trigger = event.trigger;
    if (trigger->type == TRIG_STARTUP || trigger->type == TRIG_SHUTDOWN) {
      Scheduler_handle_builtin(trigger);
    } else {
      trigger->prepare(trigger);
    }
  } while (lf_tag_compare(next_tag, self->event_queue.next_tag(&self->event_queue)) == 0);
}

/**
 * @brief Acquire a tag by iterating through all network input ports and making
 * sure that they are resolved at this tag. If the input port is unresolved we
 * must wait for the safe_to_assume_absent time before proceeding.
 *
 * @param self
 * @param next_tag
 * @return lf_ret_t
 */
lf_ret_t Scheduler_acquire_tag(Scheduler *self, tag_t next_tag) {
  Environment *env = self->env;
  Reactor *main = env->main;
  instant_t additional_sleep = 0;
  for (size_t i = 0; i < main->triggers_size; i++) {
    Trigger *trig = main->triggers[i];
    if (trig->type == TRIG_CONN_FEDERATED_INPUT) {
      FederatedInputConnection *input = (FederatedInputConnection *)trig;
      validate(input->safe_to_assume_absent == FOREVER); // TODO: We only support dataflow like things now
      // Find the max safe-to-assume-absent value and go to sleep waiting for this.
      if (lf_tag_compare(input->last_known_tag, next_tag) < 0) {
        if (input->safe_to_assume_absent > additional_sleep) {
          additional_sleep = input->safe_to_assume_absent;
        }
      }
    }
  }

  if (additional_sleep > 0) {
    instant_t now = env->get_physical_time(env);
    return env->wait_until(env, lf_time_add(now, additional_sleep));
  } else {
    return LF_OK;
  }
}

void Scheduler_run(Scheduler *self) {
  Environment *env = self->env;
  int res = 0;
  bool do_shutdown = false;
  bool keep_alive = env->keep_alive || env->has_async_events;

  if (env->has_async_events) {
    env->platform->enter_critical_section(env->platform);
  }

  while (keep_alive || !self->event_queue.empty(&self->event_queue)) {
    tag_t next_tag = self->event_queue.next_tag(&self->event_queue);
    if (lf_tag_compare(next_tag, self->env->stop_tag) > 0) {
      next_tag = self->env->stop_tag;
      do_shutdown = true;
    } else {
      do_shutdown = false;
    }

    res = self->env->wait_until(self->env, next_tag.time);
    if (res == LF_SLEEP_INTERRUPTED) {
      continue;
    } else if (res != LF_OK) {
      validate(false);
    }

    // Acquire tag
    res = Scheduler_acquire_tag(self, next_tag);
    if (res == LF_SLEEP_INTERRUPTED) {
      continue;
    }

    if (do_shutdown) {
      break;
    }

    self->prepare_timestep(self, next_tag);

    Scheduler_pop_events(self, next_tag);

    // TODO: The critical section could be smaller.
    if (env->has_async_events) {
      env->platform->leave_critical_section(env->platform);
    }

    self->run_timestep(self);
    self->clean_up_timestep(self);

    if (env->has_async_events) {
      env->platform->enter_critical_section(env->platform);
    }
  }

  self->terminate(self);
}

lf_ret_t Scheduler_schedule_at_locked(Scheduler *self, Trigger *trigger, tag_t tag) {
  Environment *env = self->env;
  Event event = {.tag = tag, .trigger = trigger};
  // Check if we are trying to schedule past stop tag
  if (lf_tag_compare(tag, env->stop_tag) > 0) {
    return LF_AFTER_STOP_TAG;
  }

  // Check if we are tring to schedule into the past
  if (lf_tag_compare(tag, env->current_tag) <= 0) {
    return LF_PAST_TAG;
  }

  return self->event_queue.insert(&self->event_queue, event);
}

lf_ret_t Scheduler_schedule_at(Scheduler *self, Trigger *trigger, tag_t tag) {
  Environment *env = self->env;

  if (env->has_async_events) {
    env->platform->enter_critical_section(env->platform);
  }

  int res = self->schedule_at_locked(self, trigger, tag);

  if (env->has_async_events) {
    env->platform->leave_critical_section(env->platform);
  }
  return res;
}

void Scheduler_ctor(Scheduler *self, Environment *env) {
  self->env = env;
  self->run = Scheduler_run;
  self->prepare_timestep = Scheduler_prepare_timestep;
  self->clean_up_timestep = Scheduler_clean_up_timestep;
  self->run_timestep = Scheduler_run_timestep;
  self->terminate = Scheduler_terminate;
  self->schedule_at = Scheduler_schedule_at;
  self->schedule_at_locked = Scheduler_schedule_at_locked;
  self->register_for_cleanup = Scheduler_register_for_cleanup;
  self->executing_tag = false;
  self->cleanup_ll_head = NULL;
  self->cleanup_ll_tail = NULL;
  EventQueue_ctor(&self->event_queue);
  ReactionQueue_ctor(&self->reaction_queue);
}
