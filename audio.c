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

  printf("A:%u\n", audio_type);

  context.index = 0;
  context.audio_type = audio_type;
  context.is_playing = 1;
  pwm_set_enabled(context.pwm_slice_id, true);
}

void audio_stop(void) {
  if (!context.is_playing) {
    return;
  }

  printf("A stop\n");
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

/*
#include "msp.h"

#include "audio/audio.h"
#include "uart/uart.h"

#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

// 8 MHz / 256
#define AUDIO_SAMPLE_RATE (31250)
#define SUCCESS_CHIME_LOOP_SIZE (15625)
#define SUCCESS_CHIME_FREQUENCY_HZ (440)
#define FAILURE_CHIME_LOOP_SIZE (23437)
#define FAILURE_CHIME_FREQUENCY_HZ (220)
#define BACKUP_ALERT_CHIME_LOOP_SIZE (31250)
#define BACKUP_ALERT_CHIME_FREQUENCY_1_HZ (110)
#define BACKUP_ALERT_CHIME_FREQUENCY_2_HZ (220)
#define SQUARE_WAVE_LOW_AMPLITUDE (30)
#define SQUARE_WAVE_HIGH_AMPLITUDE (150)

typedef uint8_t (*audio_function_t)(void);

typedef struct {
  uint8_t auto_loop;
  uint32_t loop_size;
  audio_function_t audio_function;
} audio_config_t;

typedef struct {
  volatile uint32_t index;
  volatile uint8_t *file_buffer;

  audio_type_t audio_type;
  audio_config_t audio_configs[NUM_AUDIO_TYPES];
  uint8_t is_playing;
} audio_context_t;
static audio_context_t ctx;

static void init_config(uint32_t file_size);
static uint8_t file_audio(void);
static uint8_t success_chime_audio(void);
static uint8_t failure_chime_audio(void);
static uint8_t square_wave(uint32_t frequency, uint8_t amplitude);

void audio_open(audio_open_t *open) {
  // Configure Timer_A0
  // Sourced from SMCLK (8 MHz), compare limit 255
  TIMER_A0->CTL = TIMER_A_CTL_SSEL__SMCLK;
  TIMER_A0->CCR[0] = 0xFF;
  // Enable interrupt
  TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;
  NVIC_EnableIRQ(TA0_0_IRQn);
  // Configure reset/set
  TIMER_A0->CCTL[1] = TIMER_A_CCTLN_OUTMOD_7;

  // Configure P2.4 for TIMER_A0 1 output
  P2->SEL0 |= BIT4;
  P2->SEL1 &= ~BIT4;
  P2->DIR |= BIT4;

  ctx.file_buffer = open->file_buffer;
  ctx.index = 0;
  ctx.is_playing = 0;

  init_config(open->file_size);
}

void audio_play(audio_type_t audio_type) {
  if (ctx.is_playing) {
    return;
  }

  if (audio_type == AUDIO__FILE
      && ctx.audio_configs[AUDIO__FILE].loop_size == 0) {
    audio_type = AUDIO__BACKUP_ALERT_CHIME;
  }

  uart_debug_print("A");
  uart_printx(audio_type);
  uart_debug_print("\r\n");

  ctx.index = 0;
  ctx.audio_type = audio_type;
  ctx.is_playing = 1;
  TIMER_A0->CTL |= TIMER_A_CTL_MC__UP;
}

void audio_stop(void) {
  if (!ctx.is_playing) {
    return;
  }

  uart_debug_print("A stop\r\n");

  ctx.is_playing = 0;
  TIMER_A0->CTL &= ~TIMER_A_CTL_MC_MASK;
}

void audio_set_file_size(uint32_t file_size) {
  ctx.audio_configs[AUDIO__FILE].loop_size = file_size;
}

uint8_t audio_is_playing(void) {
  return ctx.is_playing;
}

audio_type_t audio_get_type(void) {
  return ctx.audio_type;
}

void TA0_0_IRQHandler(void) {
  if (TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG) {
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

    if (ctx.is_playing) {
      TIMER_A0->CCR[1] = ctx.audio_configs[ctx.audio_type].audio_function();

      ctx.index++;
      if (ctx.index == ctx.audio_configs[ctx.audio_type].loop_size) {
        if (ctx.audio_configs[ctx.audio_type].auto_loop) {
          ctx.index = 0;
        } else {
          ctx.is_playing = 0;
          TIMER_A0->CTL &= ~TIMER_A_CTL_MC_MASK;
        }
      }
    }
  }
}

static uint8_t file_audio(void){
  return ctx.file_buffer[ctx.index];
}

static uint8_t success_chime_audio(void) {
  P1->OUT |= BIT2;
  return square_wave(SUCCESS_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
}

static uint8_t failure_chime_audio(void) {
  if (ctx.index < FAILURE_CHIME_LOOP_SIZE / 3) {
    return square_wave(FAILURE_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
  }

  if (ctx.index < 2 * (FAILURE_CHIME_LOOP_SIZE / 3)) {
    return 0;
  }

  return square_wave(FAILURE_CHIME_FREQUENCY_HZ, SQUARE_WAVE_LOW_AMPLITUDE);
}


static uint8_t backup_alert_chime_audio(void) {
  if (ctx.index < BACKUP_ALERT_CHIME_LOOP_SIZE / 2) {
    return square_wave(BACKUP_ALERT_CHIME_FREQUENCY_1_HZ, SQUARE_WAVE_HIGH_AMPLITUDE);
  }

  return square_wave(BACKUP_ALERT_CHIME_FREQUENCY_2_HZ, SQUARE_WAVE_HIGH_AMPLITUDE);
}

static uint8_t square_wave(uint32_t frequency, uint8_t amplitude) {
  return (ctx.index / (AUDIO_SAMPLE_RATE / (2 * frequency))) & 1
      ? amplitude
      : 0;
}

static void init_config(uint32_t file_size) {
  ctx.audio_configs[AUDIO__FILE].auto_loop = 1;
  ctx.audio_configs[AUDIO__FILE].loop_size = file_size;
  ctx.audio_configs[AUDIO__FILE].audio_function = file_audio;

  ctx.audio_configs[AUDIO__SUCCESS_CHIME].auto_loop = 0;
  ctx.audio_configs[AUDIO__SUCCESS_CHIME].loop_size = SUCCESS_CHIME_LOOP_SIZE;
  ctx.audio_configs[AUDIO__SUCCESS_CHIME].audio_function = success_chime_audio;

  ctx.audio_configs[AUDIO__FAILURE_CHIME].auto_loop = 0;
  ctx.audio_configs[AUDIO__FAILURE_CHIME].loop_size = FAILURE_CHIME_LOOP_SIZE;
  ctx.audio_configs[AUDIO__FAILURE_CHIME].audio_function = failure_chime_audio;

  ctx.audio_configs[AUDIO__BACKUP_ALERT_CHIME].auto_loop = 1;
  ctx.audio_configs[AUDIO__BACKUP_ALERT_CHIME].loop_size = BACKUP_ALERT_CHIME_LOOP_SIZE;
  ctx.audio_configs[AUDIO__BACKUP_ALERT_CHIME].audio_function = backup_alert_chime_audio;
}
*/
