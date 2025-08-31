#include <stdint.h>
#include <stdbool.h>

#ifndef MOCKS_H
#define MOCKS_H

// return values to be set when testing to ensure correct results
int write_pwm_return;
int reset_interrupt_return;
int write_gpio_return;
int check_uart_return;
int read_uart_return;
int read_adc_return;

#endif
