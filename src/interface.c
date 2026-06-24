#include "picosyn.h"

//#include <stdio.h>

#include "pico/stdlib.h"   // stdlib 
#include "hardware/pwm.h"  // pwm 
#include "hardware/uart.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#define LOWEST_ADC_PIN 26

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
int read_gpio(int pin) {
    return gpio_get(pin);
}

// TODO: move all similar code from picosyn.c to here
void init_i2c_interface(void) {
    // i2c communicaiton is done to 24LC256 EEPROM
    // capacity is 256kbit = 32768b = 2^15b.
    // that results in 512 64-byte pages to store data in.

    i2c_init(i2c0, 400000);
    gpio_set_function(16, GPIO_FUNC_I2C); // SDA
    gpio_set_function(17, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(16);
    gpio_pull_up(17);
}

void write_i2c(uint8_t addr, const uint8_t *src, size_t len, bool nostop) {
    i2c_write_blocking(i2c0, addr, src, len, nostop);
}

void read_i2c(uint8_t addr, uint8_t *dst, size_t len, bool nostop) {
    i2c_read_blocking(i2c0, addr, dst, len, nostop);
}

void select_adc_input(uint8_t input) {
    adc_select_input(input + LOWEST_ADC_PIN);
}

