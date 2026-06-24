#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef INTERFACE_H
#define INTERFACE_H

void write_pwm(int value);
void reset_interrupt(void);
void write_gpio(int pin, int value);
bool check_uart(void);
int read_uart(void);
int read_adc(void);
int read_gpio(int pin);
void init_i2c_interface(void);
void write_i2c(uint8_t addr, const uint8_t *src, size_t len, bool nostop);
void read_i2c(uint8_t addr, uint8_t *dst, size_t len, bool nostop);
void select_adc_input(uint8_t input);

#endif
