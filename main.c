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

#define WATER_SAMPLE_PERIOD_S (1)
#define WATER_SAMPLE_THRESHOLD (2000)

typedef struct {
} context_t;
static context_t context;

static void timer_interrupt_callback(void);

int main() {
  stdio_init_all();
  cyw43_arch_init();
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

  adc_open();
  audio_open();
  events_open();
  timer_open();

  timer_enable_event(EVENT__WATER_SAMPLE, SAMPLE_PERIOD_S, TIMER_MODE__REPEAT);

  while (1) {
    if (events_get() & EVENT__WATER_SAMPLE) {
      events_clear(EVENT__WATER_SAMPLE);
      printf("ADC: %u\n", adc_sample());
    }

    if (events_get() == 0) {
      __wfe();
    }
  }
}


