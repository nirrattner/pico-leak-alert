#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include "adc.h"

#define ADC_PIN (26)
#define ADC_INPUT (0)

void adc_open(void) {
  adc_init();
  adc_gpio_init(ADC_PIN);
  adc_select_input(ADC_INPUT);
}

uint16_t adc_get_sample(void) {
  return adc_read();
}

