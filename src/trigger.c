#include "reactor-uc/environment.h"
#include "reactor-uc/trigger.h"
#include "reactor-uc/util.h"

void Trigger_schedule_at(Trigger *self, tag_t tag) {
  assert(!self->is_scheduled);
  Event event = {.tag = tag, .trigger = self};
  self->parent->env->scheduler.event_queue.insert(&self->parent->env->scheduler.event_queue, event);
  self->is_scheduled = true;
}

void Trigger_register_effect(Trigger *self, Reaction *reaction) {
  assert(self->effects_registered < self->effects_size);
  self->effects[self->effects_registered++] = reaction;
}

void Trigger_register_source(Trigger *self, Reaction *reaction) {
  assert(self->sources_registered < self->sources_size);
  self->sources[self->sources_registered++] = reaction;
}

void Trigger_ctor(Trigger *self, TriggerType type, Reactor *parent, Reaction **effects, size_t effects_size,
                  Reaction **sources, size_t sources_size, Trigger_update_value update_value_func) {
  self->type = type;
  self->parent = parent;
  self->effects = effects;
  self->effects_size = effects_size;
  self->effects_registered = 0;
  self->sources = sources;
  self->sources_size = sources_size;
  self->sources_registered = 0;
  self->update_value = update_value_func;
  self->next = NULL;
  self->is_present = false;
  self->is_scheduled = false;

  self->schedule_at = Trigger_schedule_at;
  self->register_effect = Trigger_register_effect;
  self->register_source = Trigger_register_source;
}
