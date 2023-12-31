#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "audio.h"
#include "events.h"
#include "timer.h"

typedef struct {
} context_t;
static context_t context;

static void timer_interrupt_callback(void);

int main() {
  stdio_init_all();

  // ADC
  // uint16_t adc_sample;
  // adc_init();
  // adc_select_input(0);

  cyw43_arch_init();
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

  audio_open();
  events_open();
  timer_open();

  timer_enable_event(EVENT__WATER_SAMPLE, 1000, TIMER_MODE__REPEAT);

  while (1) {
    if (events_get() & EVENT__WATER_SAMPLE) {
      printf("SAMPLE\n");
      events_clear(EVENT__WATER_SAMPLE);
    }

    if (events_get() == 0) {
      __wfe();
    }
  }
}


