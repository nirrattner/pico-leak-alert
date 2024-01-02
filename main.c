#include <stdio.h>

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "adc.h"
#include "audio.h"
#include "events.h"
#include "timer.h"

#define SIREN_AUDIO_DURATION_S (30)
#define WATER_SAMPLE_PERIOD_S (1)
#define WATER_ADC_THRESHOLD (1800)

// typedef struct {
// } context_t;
// static context_t context;

static void event_water_sample(void);
static void event_siren_done(void);

int main() {
  stdio_init_all();
  cyw43_arch_init();
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

  adc_open();
  audio_open();
  events_open();
  timer_open();

  timer_enable_event(EVENT__WATER_SAMPLE, WATER_SAMPLE_PERIOD_S, TIMER_MODE__REPEAT);

  while (1) {
    if (events_get() & EVENT__WATER_SAMPLE) {
      events_clear(EVENT__WATER_SAMPLE);
      event_water_sample();
    }

    if (events_get() & EVENT__SIREN_DONE) {
      events_clear(EVENT__SIREN_DONE);
      event_siren_done();
    }

    if (events_get() == 0) {
      __wfe();
    }
  }
}

static void event_water_sample(void) {
  printf("ADC: %u\n", adc_get_sample());

  if (adc_get_sample() > WATER_ADC_THRESHOLD) {
    timer_enable_event(EVENT__SIREN_DONE, SIREN_AUDIO_DURATION_S, TIMER_MODE__ONCE);
    audio_play(AUDIO__SIREN);
  }
}

static void event_siren_done(void) {
  audio_stop();
}

