#include "reactor-uc/environment.h"
#include "reactor-uc/logging.h"
#include "reactor-uc/network_channel.h"
#include "reactor-uc/reactor.h"
#include "reactor-uc/scheduler.h"
#include <assert.h>
#include <inttypes.h>

void Environment_assemble(Environment *self) { validaten(self->main->calculate_levels(self->main)); }

// Find the start time of the federation.
// FIXME: This needs to involve communcation with the other federates. Currently hardcoded to 1 second.
void Environment_set_start_time(Environment *self) {
#if defined(PLATFORM_POSIX)
  self->start_time = ((self->platform->get_physical_time(self->platform) + SEC(1)) / SEC(1)) * SEC(1);
#elif defined(PLATFORM_RIOT)
  self->start_time = SEC(1);
#elif defined(PLATFORM_ZEPHY)
  self->start_time = SEC(1);
#endif
  LF_INFO(ENV, "Start time: %" PRId64, self->start_time);
}

void Environment_start(Environment *self) {
  Environment_set_start_time(self);
  self->scheduler.run(&self->scheduler);
}

lf_ret_t Environment_wait_until(Environment *self, instant_t wakeup_time) {
  if (wakeup_time < self->get_physical_time(self)) {
    return LF_OK;
  }

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
  self->set_start_time = Environment_set_start_time;

  self->keep_alive = false;
  self->has_async_events = false;
  self->startup = NULL;
  self->shutdown = NULL;
  self->stop_tag = FOREVER_TAG;
  self->start_time = self->platform->get_physical_time(
      self->platform); // TODO: This should be done in set_start_time. Put it here now for backwards compatability.
  Scheduler_ctor(&self->scheduler, self);
  self->current_tag = NEVER_TAG;
}

void Environment_free(Environment *self) {
  (void)self;
  LF_INFO(ENV, "Freeing environment");
  for (size_t i = 0; i < self->bundles_size; i++) {
    NetworkChannel *chan = self->bundles[i]->net_channel;
    chan->free(chan);
  }
}