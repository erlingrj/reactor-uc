#ifndef REACTOR_UC_SCHEDULER_H
#define REACTOR_UC_SCHEDULER_H

#include "reactor-uc/queues.h"

typedef struct Scheduler Scheduler;
typedef struct Environment Environment;

struct Scheduler {
  Environment *env;
  EventQueue event_queue;
  ReactionQueue reaction_queue;
  Trigger *cleanup_ll_head;
  Trigger *cleanup_ll_tail;
  bool executing_tag;
  int (*schedule_at)(Scheduler *self, Trigger *trigger, tag_t tag);
  int (*schedule_at_locked)(Scheduler *self, Trigger *trigger, tag_t tag);
  void (*run)(Scheduler *self);
  void (*prepare_timestep)(Scheduler *self, tag_t tag);
  void (*clean_up_timestep)(Scheduler *self);
  void (*run_timestep)(Scheduler *self);
  void (*terminate)(Scheduler *self);
  void (*register_for_cleanup)(Scheduler *self, Trigger *trigger);
};

void Scheduler_ctor(Scheduler *self, Environment *env);
#endif
