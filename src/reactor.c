#include "reactor-uc/builtin_triggers.h"
#include "reactor-uc/environment.h"
#include "reactor-uc/reactor.h"

#include <string.h>

void Reactor_register_startup(Reactor *self, Startup *startup) {
  (void)self;
  startup->super.schedule_at((Trigger *)startup, ZERO_TAG);
}

void Reactor_calculate_levels(Reactor *self) {
  for (size_t i = 0; i < self->reactions_size; i++) {
    size_t level = self->reactions[i]->get_level(self->reactions[i]);
    (void)level;
  }

  for (size_t i = 0; i < self->children_size; i++) {
    Reactor_calculate_levels(self->children[i]);
  }
}

void Reactor_ctor(Reactor *self, const char *name, Environment *env, Reactor **children, size_t children_size,
                  Reaction **reactions, size_t reactions_size, Trigger **triggers, size_t triggers_size) {

  strncpy(self->name, name, REACTOR_NAME_MAX_LEN);
  self->env = env;
  self->children = children;
  self->children_size = children_size;
  self->triggers = triggers;
  self->triggers_size = triggers_size;
  self->reactions = reactions;
  self->reactions_size = reactions_size;
  self->register_startup = Reactor_register_startup;
  self->calculate_levels = Reactor_calculate_levels;
}
