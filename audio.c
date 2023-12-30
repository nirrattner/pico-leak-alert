#include <stdio.h>

#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

#include "audio.h"

#define AUDIO_PWM_PIN (0)
#define AUDIO_PWM_TICKS (250)
#define AUDIO_SAMPLE_FREQUENCY_HZ (50000)

#define SUCCESS_CHIME_LOOP_SIZE (AUDIO_SAMPLE_FREQUENCY_HZ / 2)
#define SUCCESS_CHIME_FREQUENCY_HZ (440)
#define SQUARE_WAVE_HIGH_AMPLITUDE (150)

typedef struct {
  volatile uint32_t audio_index;
  uint8_t is_playing;
  uint32_t pwm_slice_id;
} audio_context_t;
static audio_context_t context;

static uint8_t success_chime_audio(void);
static uint8_t square_wave(uint32_t frequency, uint8_t amplitude);
static void pwm_interrupt_callback(void);

void audio_open(void) {
  context.audio_index = 0;
  context.is_playing = 0;
  context.pwm_slice_id = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);

  gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_callback);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_clear_irq(context.pwm_slice_id);
  pwm_set_irq_enabled(context.pwm_slice_id, true);

  pwm_set_wrap(context.pwm_slice_id, AUDIO_PWM_TICKS - 1);
  pwm_set_clkdiv_int_frac(context.pwm_slice_id, 10, 0);

  pwm_set_enabled(context.pwm_slice_id, true);
}

static uint8_t success_chime_audio(void) {
  if (context.audio_index < SUCCESS_CHIME_LOOP_SIZE / 2) {
    return 0;
  }
  return square_wave(SUCCESS_CHIME_FREQUENCY_HZ, SQUARE_WAVE_HIGH_AMPLITUDE);
}


static uint8_t square_wave(uint32_t frequency, uint8_t amplitude) {
  return (context.audio_index / (AUDIO_SAMPLE_FREQUENCY_HZ / (2 * frequency))) & 1
      ? amplitude
      : 0;
}

static void pwm_interrupt_callback(void) {
  pwm_clear_irq(context.pwm_slice_id);
  pwm_set_gpio_level(AUDIO_PWM_PIN, success_chime_audio());

  context.audio_index = (context.audio_index + 1) % SUCCESS_CHIME_LOOP_SIZE;
  if (context.audio_index == 0) {
    printf("T\n");
  }
}

