#include "reactor-uc/builtin_triggers.h"
#include "reactor-uc/environment.h"
#include "reactor-uc/reaction.h"
#include "reactor-uc/reactor.h"
#include <stdio.h>

typedef struct {
  Startup super;
  Reaction *effects_[1];
} MyStartup;

typedef struct {
  Reaction super;
} ProducerReaction;

typedef struct {
  Reactor super;
  ProducerReaction my_reaction;
  MyStartup startup;
  Reaction *_reactions[1];
  Trigger *_triggers[1];
  int cnt;
} MyReactor;

void MyStartup_ctor(MyStartup *self, Reactor *parent, Reaction *effects) {
  self->effects_[0] = effects;
  Startup_ctor(&self->super, parent, self->effects_, 1);
}

int startup_handler(Reaction *_self) {
  Producer *self = (Producer *)_self->parent;
  (void)self;
  printf("Hello World\n");
  return 0;
}

void MyReaction_ctor(ProducerReaction *self, Reactor *parent) {
  Reaction_ctor(&self->super, parent, startup_handler, NULL, 0);
  self->super.level = 0;
}

void MyReactor_ctor(Producer *self, Environment *env) {
  self->reactions[0] = (Reaction *)&self->producer_reaction;
  self->triggers[0] = (Trigger *)&self->startup;
  Reactor_ctor(&self->super, env, NULL, 0, self->reactions, 1, self->triggers, 1);
  MyReaction_ctor(&self->producer_reaction, &self->super);
  MyStartup_ctor(&self->startup, &self->super, &self->producer_reaction.super);
  self->super.register_startup(&self->super, &self->startup.super);
}

int main() {
  Environment env;
  Producer my_reactor;
  Environment_ctor(&env, (Reactor *)&my_reactor);
  MyReactor_ctor(&my_reactor, &env);
  env.assemble(&env);
  env.start(&env);
}
