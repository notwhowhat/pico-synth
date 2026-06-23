#include "interface.h"
#include "mocks.h"

void write_pwm(int value) {
}
void reset_interrupt(void) {
}
void write_gpio(int pin, int value) {
}
bool check_uart(void) {
    return check_uart_return;
}
int read_uart(void) {
    return read_uart_return;
}
int read_adc(void) {
    return read_adc_return;
}
int read_gpio(pin) {
    return 1023;
}

void init_i2c_interface(void) {
}

void write_i2c(uint8_t addr, const uint8_t *src, size_t len, bool nostop) {
}

void read_i2c(uint8_t addr, uint8_t *dst, size_t len, bool nostop) {
}

