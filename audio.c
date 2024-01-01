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
#define FAILURE_CHIME_LOOP_SIZE (3 * AUDIO_SAMPLE_FREQUENCY_HZ / 2)
#define FAILURE_CHIME_FREQUENCY_HZ (220)
#define SIREN_LOOP_SIZE (AUDIO_SAMPLE_FREQUENCY_HZ)
#define SIREN_FREQUENCY_1_HZ (110)
#define SIREN_FREQUENCY_2_HZ (220)

#define SQUARE_WAVE_HIGH_AMPLITUDE (150)
#define SQUARE_WAVE_LOW_AMPLITUDE (30)

typedef uint8_t (*audio_function_t)(void);

typedef struct {
  uint8_t auto_loop;
  uint32_t loop_size;
  audio_function_t audio_function;
} audio_config_t;

typedef struct {
  volatile uint32_t index;

  audio_type_t audio_type;
  audio_config_t audio_configs[NUM_AUDIO_TYPES];
  uint32_t pwm_slice_id;
  uint8_t is_playing;
} audio_context_t;
static audio_context_t context;

static void initialize_audio_configs(void);
static uint8_t success_chime_audio(void);
static uint8_t failure_chime_audio(void);
static uint8_t siren_audio(void);
static uint8_t square_wave(uint32_t frequency, uint8_t amplitude);
static void pwm_interrupt_callback(void);

void audio_open(void) {
  initialize_audio_configs();

  context.index = 0;
  context.is_playing = 0;
  context.pwm_slice_id = pwm_gpio_to_slice_num(AUDIO_PWM_PIN);

  gpio_set_function(AUDIO_PWM_PIN, GPIO_FUNC_PWM);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_callback);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  pwm_clear_irq(context.pwm_slice_id);
  pwm_set_irq_enabled(context.pwm_slice_id, true);

  pwm_set_wrap(context.pwm_slice_id, AUDIO_PWM_TICKS - 1);
  pwm_set_clkdiv_int_frac(context.pwm_slice_id, 10, 0);
}

void audio_play(audio_type_t audio_type) {
  if (context.is_playing) {
    return;
  }

  context.index = 0;
  context.audio_type = audio_type;
  context.is_playing = 1;
  pwm_set_enabled(context.pwm_slice_id, true);
}

void audio_stop(void) {
  if (!context.is_playing) {
    return;
  }

  context.is_playing = 0;
  pwm_set_enabled(context.pwm_slice_id, true);
}

static void initialize_audio_configs(void) {
  context.audio_configs[AUDIO__SUCCESS_CHIME].auto_loop = 0;
  context.audio_configs[AUDIO__SUCCESS_CHIME].loop_size = SUCCESS_CHIME_LOOP_SIZE;
  context.audio_configs[AUDIO__SUCCESS_CHIME].audio_function = success_chime_audio;

  context.audio_configs[AUDIO__FAILURE_CHIME].auto_loop = 0;
  context.audio_configs[AUDIO__FAILURE_CHIME].loop_size = FAILURE_CHIME_LOOP_SIZE;
  context.audio_configs[AUDIO__FAILURE_CHIME].audio_function = failure_chime_audio;

  context.audio_configs[AUDIO__SIREN].auto_loop = 1;
  context.audio_configs[AUDIO__SIREN].loop_size = SIREN_LOOP_SIZE;
  context.audio_configs[AUDIO__SIREN].audio_function = siren_audio;
}

static uint8_t success_chime_audio(void) {
  return square_wave(SUCCESS_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
}

static uint8_t failure_chime_audio(void) {
  if (context.index < FAILURE_CHIME_LOOP_SIZE / 3) {
    return square_wave(FAILURE_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
  }

  if (context.index < 2 * (FAILURE_CHIME_LOOP_SIZE / 3)) {
    return 0;
  }

  return square_wave(FAILURE_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
}

static uint8_t siren_audio(void) {
  if (context.index < SIREN_LOOP_SIZE / 2) {
    return square_wave(SIREN_FREQUENCY_1_HZ, SQUARE_WAVE_HIGH_AMPLITUDE);
  }

  return square_wave(SIREN_FREQUENCY_2_HZ, SQUARE_WAVE_HIGH_AMPLITUDE);
}

static uint8_t square_wave(uint32_t frequency, uint8_t amplitude) {
  return (context.index / (AUDIO_SAMPLE_FREQUENCY_HZ / (2 * frequency))) & 1
      ? amplitude
      : 0;
}

static void pwm_interrupt_callback(void) {
  pwm_clear_irq(context.pwm_slice_id);

  if (context.is_playing == 0) {
    return;
  }

  pwm_set_gpio_level(AUDIO_PWM_PIN, context.audio_configs[context.audio_type].audio_function());
  context.index++;

  if (context.index != context.audio_configs[context.audio_type].loop_size) {
    return;
  }

  if (context.audio_configs[context.audio_type].auto_loop) {
    context.index = 0;
    return;
  } 

  context.is_playing = 0;
  pwm_set_enabled(context.pwm_slice_id, false);
}

