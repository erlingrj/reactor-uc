#include "reactor-uc/reactor-uc.h"

typedef struct {
  LogicalAction super;
  int value;
  int next_value;
  Reaction *sources[1];
  Reaction *effects[1];
} MyAction;

typedef struct MyStartup MyStartup;
struct MyStartup {
  Startup super;
  Reaction *effects_[1];
};

typedef struct {
  Reaction super;
  Trigger *effects[1];
} MyReaction;

struct MyReactor {
  Reactor super;
  MyReaction my_reaction;
  MyAction my_action;
  MyStartup startup;
  Reaction *_reactions[1];
  Trigger *_triggers[2];
  int cnt;
};

void MyAction_ctor(MyAction *self, struct MyReactor *parent) {
  self->sources[0] = &parent->my_reaction.super;
  self->effects[0] = &parent->my_reaction.super;
  LogicalAction_ctor(&self->super, MSEC(0), &parent->super, self->sources, 1, self->effects, 1, &self->value,
                     &self->next_value, sizeof(self->value));
}

void MyStartup_ctor(struct MyStartup *self, Reactor *parent, Reaction *effects) {
  self->effects_[0] = effects;
  Startup_ctor(&self->super, parent, self->effects_, 1);
}

int action_handler(Reaction *_self) {
  struct MyReactor *self = (struct MyReactor *)_self->parent;
  MyAction *my_action = &self->my_action;

  printf("Hello World\n");
  printf("Action = %d\n", my_action->value);
  lf_schedule(my_action, self->cnt++, SEC(1));
  return 0;
}

void MyReaction_ctor(MyReaction *self, Reactor *parent) {
  Reaction_ctor(&self->super, parent, action_handler, self->effects, 1, 0);
}

void MyReactor_ctor(struct MyReactor *self, Environment *env) {
  self->_reactions[0] = (Reaction *)&self->my_reaction;
  self->_triggers[0] = (Trigger *)&self->startup;
  self->_triggers[1] = (Trigger *)&self->my_action;
  Reactor_ctor(&self->super, "MyReactor", env, NULL, 0, self->_reactions, 1, self->_triggers, 2);
  MyAction_ctor(&self->my_action, self);
  MyReaction_ctor(&self->my_reaction, &self->super);
  MyStartup_ctor(&self->startup, &self->super, &self->my_reaction.super);
  self->my_action.super.super.register_effect(&self->my_action.super.super, &self->my_reaction.super);
  self->my_reaction.super.register_effect(&self->my_reaction.super, &self->my_action.super.super);

  self->my_action.super.super.register_source(&self->my_action.super.super, &self->my_reaction.super);
  self->super.register_startup(&self->super, &self->startup.super);
  self->cnt = 0;
}

int main() {
  struct MyReactor my_reactor;
  Environment env;
  Environment_ctor(&env, (Reactor *)&my_reactor);
  MyReactor_ctor(&my_reactor, &env);
  env.assemble(&env);
  env.start(&env);
}
