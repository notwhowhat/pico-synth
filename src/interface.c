#include "picosyn.h"

//#include <stdio.h>

#include "pico/stdlib.h"   // stdlib 
#include "hardware/pwm.h"  // pwm 
#include "hardware/uart.h"

void write_pwm(int value) {
    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)value);
}
void reset_interrupt(void) {
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    
}

void write_gpio(int pin, int value) {
    gpio_put(pin, value);
}
bool check_uart(void) {
    return uart_is_readable(uart1);
}
int read_uart(void) {
    return uart_getc(uart1);
}
int read_adc(void) {
    return adc_read();
}

