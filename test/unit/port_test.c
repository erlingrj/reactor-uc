#include "reactor-uc/reactor-uc.h"
#include "unity.h"

// Reactor Sender
typedef struct {
  Timer super;
  Reaction *effects[1];
} Timer1;

typedef struct {
  Reaction super;
} Reaction1;

typedef struct {
  Output super;
  Reaction *sources[1];
} Out;

struct Sender {
  Reactor super;
  Reaction1 reaction;
  Timer1 timer;
  Out out;
  Reaction *_reactions[1];
  Trigger *_triggers[1];
};

void timer_handler(Reaction *_self) {
  struct Sender *self = (struct Sender *)_self->parent;
  Environment *env = self->super.env;
  Out *out = &self->out;

  printf("Timer triggered @ %ld\n", env->get_elapsed_logical_time(env));
  lf_set(out, env->get_elapsed_logical_time(env));
}

void Reaction1_ctor(Reaction1 *self, Reactor *parent) {
  Reaction_ctor(&self->super, parent, timer_handler, NULL, 0, 0);
}

void Out_ctor(Out *self, struct Sender *parent) {
  self->sources[0] = &parent->reaction.super;
  Output_ctor(&self->super, &parent->super, self->sources, 1);
}

void Sender_ctor(struct Sender *self, Reactor *parent, Environment *env) {
  self->_reactions[0] = (Reaction *)&self->reaction;
  self->_triggers[0] = (Trigger *)&self->timer;
  Reactor_ctor(&self->super, "Sender", env, parent, NULL, 0, self->_reactions, 1, self->_triggers, 1);
  Reaction1_ctor(&self->reaction, &self->super);
  Timer_ctor(&self->timer.super, &self->super, 0, SEC(1), self->timer.effects, 1);
  Out_ctor(&self->out, self);
  TIMER_REGISTER_EFFECT(self->timer, self->reaction);
  OUTPUT_REGISTER_SOURCE(self->out, self->reaction);
}

// Reactor Receiver
typedef struct {
  Reaction super;
} Reaction2;

typedef struct {
  Input super;
  instant_t buffer[1];
  Reaction *effects[1];
} In;

struct Receiver {
  Reactor super;
  Reaction2 reaction;
  In inp;
  Reaction *_reactions[1];
  Trigger *_triggers[1];
};

void In_ctor(In *self, struct Receiver *parent) {
  Input_ctor(&self->super, &parent->super, self->effects, 1, self->buffer, sizeof(self->buffer[0]));
}

void input_handler(Reaction *_self) {
  struct Receiver *self = (struct Receiver *)_self->parent;
  Environment *env = self->super.env;
  In *inp = &self->inp;

  printf("Input triggered @ %ld with %ld\n", env->get_elapsed_logical_time(env), lf_get(inp));
  TEST_ASSERT_EQUAL(lf_get(inp), env->get_elapsed_logical_time(env));
}

void Reaction2_ctor(Reaction2 *self, Reactor *parent) {
  Reaction_ctor(&self->super, parent, input_handler, NULL, 0, 0);
}

void Receiver_ctor(struct Receiver *self, Reactor *parent, Environment *env) {
  self->_reactions[0] = (Reaction *)&self->reaction;
  self->_triggers[0] = (Trigger *)&self->inp;
  Reactor_ctor(&self->super, "Receiver", env, parent, NULL, 0, self->_reactions, 1, self->_triggers, 1);
  Reaction2_ctor(&self->reaction, &self->super);
  In_ctor(&self->inp, self);

  // Register reaction as an effect of in
  INPUT_REGISTER_EFFECT(self->inp, self->reaction);
}

struct Conn1 {
  LogicalConnection super;
  Input *downstreams[1];
};

void Conn1_ctor(struct Conn1 *self, Reactor *parent, Output *upstream) {
  LogicalConnection_ctor(&self->super, parent, &upstream->super, (Port **)self->downstreams, 1);
}

// Reactor main
struct Main {
  Reactor super;
  struct Sender sender;
  struct Receiver receiver;
  struct Conn1 conn;

  Reactor *_children[2];
};

void Main_ctor(struct Main *self, Environment *env) {
  self->_children[0] = &self->sender.super;
  Sender_ctor(&self->sender, &self->super, env);

  self->_children[1] = &self->receiver.super;
  Receiver_ctor(&self->receiver, &self->super, env);

  Conn1_ctor(&self->conn, &self->super, &self->sender.out.super);
  self->conn.super.super.register_downstream(&self->conn.super.super, &self->receiver.inp.super.super);

  Reactor_ctor(&self->super, "Main", env, NULL, self->_children, 2, NULL, 0, NULL, 0);
}

void test_simple() {
  static const uint8_t data[100] = {0};
  printf("data: %p\n", data);
  struct Main main;
  Environment env;
  Environment_ctor(&env, (Reactor *)&main);
  Main_ctor(&main, &env);
  env.set_timeout(&env, SEC(1));
  env.assemble(&env);
  env.start(&env);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_simple);
  return UNITY_END();
}