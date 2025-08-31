#include <stdint.h>
#include <stdbool.h>

#ifndef INTERFACE_H
#define INTERFACE_H

void write_pwm(int value);
void reset_interrupt(void);
void write_gpio(int pin, int value);
bool check_uart(void);
int read_uart(void);
int read_adc(void);

#endif
