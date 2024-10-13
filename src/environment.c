#include "reactor-uc/environment.h"
#include "reactor-uc/logging.h"
#include "reactor-uc/platform/posix/tcp_ip_channel.h" // FIXME: NetworkChannel instead
#include "reactor-uc/reactor.h"
#include "reactor-uc/scheduler.h"
#include <assert.h>

static struct UsingSomeData {
  uint8_t array[100];
  Environment env;
} data[3];

void Environment_assemble(Environment *self) { validaten(self->main->calculate_levels(self->main)); }

void Environment_start(Environment *self) { self->scheduler.run(&self->scheduler); }

void not_useful_function(const char *msg) {
  static bool bools[19];
  for (uint32_t i = 0; i < sizeof(bools); i++) {
    if (bools[i] == true) {
      puts(msg);
      bools[i] = false;
    } else {
      bools[i] = true;
      data[0] = (struct UsingSomeData) {
        .array = { 0 }
      };
    }
  }
}

lf_ret_t Environment_wait_until(Environment *self, instant_t wakeup_time) {
  if (wakeup_time < self->get_physical_time(self)) {
    return LF_OK;
  }
  not_useful_function("Testing");
  if (self->has_async_events) {
    return self->platform->wait_until_interruptable(self->platform, wakeup_time);
  } else {
    return self->platform->wait_until(self->platform, wakeup_time);
  }
}

void Environment_set_timeout(Environment *self, interval_t duration) {
  self->stop_tag.microstep = 0;
  self->stop_tag.time = self->start_time + duration;
}

interval_t Environment_get_logical_time(Environment *self) { return self->current_tag.time; }
interval_t Environment_get_elapsed_logical_time(Environment *self) { return self->current_tag.time - self->start_time; }
interval_t Environment_get_physical_time(Environment *self) {
  return self->platform->get_physical_time(self->platform);
}
interval_t Environment_get_elapsed_physical_time(Environment *self) {
  return self->platform->get_physical_time(self->platform) - self->start_time;
}

void Environment_ctor(Environment *self, Reactor *main) {
  self->main = main;
  self->platform = Platform_new();
  Platform_ctor(self->platform);
  self->platform->initialize(self->platform);

  self->assemble = Environment_assemble;
  self->start = Environment_start;
  self->wait_until = Environment_wait_until;
  self->set_timeout = Environment_set_timeout;
  self->get_elapsed_logical_time = Environment_get_elapsed_logical_time;
  self->get_logical_time = Environment_get_logical_time;
  self->get_physical_time = Environment_get_physical_time;
  self->get_elapsed_physical_time = Environment_get_elapsed_physical_time;

  not_useful_function("Hello");

  self->keep_alive = false;
  self->has_async_events = false;
  self->startup = NULL;
  self->shutdown = NULL;
  self->stop_tag = FOREVER_TAG;
  Scheduler_ctor(&self->scheduler, self);
  self->current_tag = NEVER_TAG;

  // Set start time
  // TODO: This must be resolved in the federation. Currently set start tag to nearest second.
  self->start_time = ((self->platform->get_physical_time(self->platform) + SEC(1)) / SEC(1)) * SEC(1);
  LF_INFO(ENV, "Start time: %" PRId64, self->start_time);
}

void Environment_free(Environment *self) {
  (void)self;
  LF_INFO(ENV, "Freeing environment");
}