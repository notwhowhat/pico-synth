#include "mocks.h"

#include "midi.h"
#include "voice.h"

#include <stdio.h>

int main(void) {
    on_pwm_interrupt();

    printf("hello world");
    return 0;
}
