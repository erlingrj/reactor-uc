#include "reactor-uc/port.h"
#include "reactor-uc/reaction.h"
#include "reactor-uc/trigger.h"
#include "reactor-uc/util.h"

void Reaction_register_effect(Reaction *self, Trigger *effect) {
  assert(self->effects_registered < self->effects_size);
  self->effects[self->effects_registered++] = effect;
}

size_t Reaction_get_level(Reaction *self) {
  if (self->level < 0) {
    self->level = self->calculate_level(self);
  }
  return self->level;
}

// FIXME: Detect causality cycle also here.
size_t Reaction_calculate_level(Reaction *self) {
  size_t max_level = 0;

  // Possibly inherit level from reactions within same reactor with precedence.
  if (self->index >= 1) {
    Reaction *reaction_prev_index = self->parent->reactions[self->index - 1];
    size_t prev_level = reaction_prev_index->get_level(reaction_prev_index);
    if (prev_level > max_level) {
      max_level = prev_level;
    }
  }

  // Find sources of this reaction by searching through all triggers of parent
  for (size_t i = 0; i < self->parent->triggers_size; i++) {
    Trigger *trigger = self->parent->triggers[i];
    if (trigger->type == INPUT) {
      for (size_t j = 0; j < trigger->effects_size; j++) {
        if (trigger->effects[j] == self) {
          InputPort *port = (InputPort *)trigger;
          if (port->super.conn_in) {
            OutputPort *final_upstream_port = port->super.conn_in->get_final_upstream(port->super.conn_in);
            for (size_t k = 0; k < final_upstream_port->super.super.sources_size; k++) {
              Reaction *upstream = final_upstream_port->super.super.sources[k];
              size_t upstream_level = upstream->get_level(upstream) + 1;
              if (upstream_level > max_level) {
                max_level = upstream_level;
              }
            }
          }
        }
      }
    }
  }

  return max_level;
}

void Reaction_ctor(Reaction *self, Reactor *parent, ReactionHandler body, Trigger **effects, size_t effects_size,
                   size_t index) {
  self->body = body;
  self->parent = parent;
  self->effects = effects;
  self->effects_size = effects_size;
  self->effects_registered = 0;
  self->register_effect = Reaction_register_effect;
  self->calculate_level = Reaction_calculate_level;
  self->get_level = Reaction_get_level;
  self->index = index;
  self->level = -1;
}
