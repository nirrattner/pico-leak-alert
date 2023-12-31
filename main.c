#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "audio.h"

#define TIMER_ID (0)
#define TIMER_PERIOD_US (500000)

#define TONE_EVENT_COUNT (4)

typedef enum {
  EVENT__NONE = 0,
  EVENT__LED_TOGGLE = (1 << 0),
  EVENT__TONE = (1 << 2),
} event_t;

typedef struct {
  volatile event_t event;
  volatile uint32_t timer_expiry;
  volatile uint32_t audio_index;
  uint32_t pwm_slice_id;
  uint8_t led_state;
  uint32_t tone_counter;
} context_t;
static context_t context;

static void event_led_toggle(void);
static void timer_interrupt_callback(void);
static void pwm_interrupt_callback(void);

int main() {
  stdio_init_all();
  printf("Starting...\n");

  cyw43_arch_init();

  // ADC
  // uint16_t adc_sample;
  // adc_init();
  // adc_select_input(0);

  context.event = EVENT__NONE;
  context.audio_index = 0;

  context.tone_counter = 0;

  // LED
  context.led_state = 1;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, context.led_state);

  // TIMER
  irq_set_exclusive_handler(TIMER_IRQ_0, timer_interrupt_callback);
  irq_set_enabled(TIMER_IRQ_0, true);
  context.timer_expiry = timer_hw->timerawl + TIMER_PERIOD_US;
  timer_hw->alarm[TIMER_ID] = context.timer_expiry;
  hw_set_bits(&timer_hw->inte, 1u << TIMER_ID);

  audio_open();

  while (1) {
    if (context.event & EVENT__LED_TOGGLE) {
      event_led_toggle();
      context.event &= ~EVENT__LED_TOGGLE;
    }
    if (context.event & EVENT__TONE) {
      audio_play(AUDIO__FAILURE_CHIME);
      context.event &= ~EVENT__TONE;
    }

    if (context.event == 0) {
      __wfe();
    }
  }
}

static void event_led_toggle(void) {
  // printf("led_toggle\n");
  context.led_state ^= 1;
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, context.led_state);
}

static void timer_interrupt_callback(void) {
  hw_clear_bits(&timer_hw->intr, 1u << TIMER_ID);
  context.event |= EVENT__LED_TOGGLE;

  context.timer_expiry += TIMER_PERIOD_US;
  timer_hw->alarm[TIMER_ID] = context.timer_expiry;

  context.tone_counter++;
  if (context.tone_counter == TONE_EVENT_COUNT) {
    context.tone_counter = 0;
    context.event |= EVENT__TONE;
  }
}

