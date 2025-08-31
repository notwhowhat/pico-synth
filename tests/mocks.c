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