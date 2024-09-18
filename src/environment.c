#include "reactor-uc/environment.h"
#include "reactor-uc/reactor.h"
#include "reactor-uc/scheduler.h"

#include <assert.h>
#include <stdio.h>

Environment lf_global_env;

void Environment_terminate(void) {
  // TODO: Careful here with catching Ctrl+D and just calling terminate...
  lf_global_env.scheduler.terminate(&lf_global_env.scheduler);
}

void Environment_assemble(Environment *self) {
  printf("Assembling environment\n");
  self->current_tag.microstep = 0;
  self->current_tag.time = 0;

  printf("Assigning levels\n");
  self->main->calculate_levels(self->main);

  if (atexit(Environment_terminate) != 0) {
    printf("Could not register termination function\n");
    assert(false);
  }
}

void Environment_start(Environment *self) {
  (void)self;
  printf("Running program\n");

  self->scheduler.run(&self->scheduler);
}

int Environment_wait_until(Environment *self, instant_t wakeup_time) {
  (void)self;
  (void)wakeup_time;
  return 0;
}

void Environment_ctor(Environment *self, Reactor *main) {
  self->main = main;
  self->assemble = Environment_assemble;
  self->start = Environment_start;
  self->keep_alive = false;
  self->wait_until = Environment_wait_until;
  self->startup = NULL;
  self->shutdown = NULL;
  self->stop_tag = FOREVER_TAG;
  Scheduler_ctor(&self->scheduler, self);
}
