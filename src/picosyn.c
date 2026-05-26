#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "pico/multicore.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#include "picosyn.h"
#include "voice.h"
#include "midi.h"
#include "controls.h"

#include "math.h"
// a permanent led pin for checking if any runtime errors have occured without a debugger
//#define ERR_LED_PIN 25
//#define MIDI_LED_PIN 15
//#define AUDIO_PIN 2 

#define MUX_1_PIN 17
#define MUX_2_PIN 19
#define MUX_3_PIN 20 

extern inline fixed mul_fixed(fixed a, fixed b);
extern inline fixed div_fixed(fixed a, fixed b);

/*
notes:
the table for increments might not be necessary.
it would make lfos more easy to implement.

I HAVE AN IDEA WOOO!
different music modules can be named after birds. (i could just make it animals in general)
this: hummingbird maybe
sampler: parrot(cause it repeats things)
sequencer: woodpecker maybe cause' they like to bang their beaks rythmically.
otherwies it could be bats. they have like sonar.
any effect units could be the birds' environments, like a cave for echo.

*/

/* 
time to implement multithreading! it will be split up in two threads. the load
won't be balanced in any way, instead, the midi will be on one core, and the audio processing will be on the
other one. this will probably be the best solution because the audio is triggered by an isr.

i will share the voices in global memory. it's suboptimal, but i will at least be using mutexes instead.
where should i the audio get processed? probably in the second core? idk.
*/

/*
envelopes 2.0
when the envelopes are reset they will:
change the period/part (attack, for example), but not skip periods. they will only continue.
decay is a bit weird, because the sustain can be changed, which results in sometimes going up. this is a result of
the desired operation
*/


void core1_entry(void) {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    // XXX: this should probably be after the next chunk of code so the interrupt doesn't get run too early!
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_interrupt); // TODO: make it use process_midi
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga8478ee26cc144e947ccd75b0169059a6

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();
    // these values are set to run at the sample rate of 44.1KHz.
    pwm_config_set_clkdiv(&config, 14.0f); 
    pwm_config_set_wrap(&config, 437); 
    pwm_init(audio_pin_slice, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    while (1) {
        tight_loop_contents();
    }
}

void debug_message(void) {
    int used_voices = 0;
    for (int i = 0; i < VOICE_COUNT; i++) {
        if (voices[i].used == true) {
            used_voices++;
        }
    }
    printf("used: %d\n", used_voices);
}


// I SHOULD RUN THE MIDIIII ON A UART INTERRUPT WHY CAN'T I THINKGKKKGKGKGK
int main(void) {
    /* Overclocking for fun but then also so the system clock is a 
     * multiple of typical audio sampling rates.
     */
    stdio_init_all();

    // TODO: its the clock frequency that's wrong. at just a tiny bit more it works flawelessly, but 
    // if i just lower it a tiny bit it errors, because of no led output. 
    //set_sys_clock_khz(123480, true); 
    //set_sys_clock_khz(124000, true);
    // at the higher clock speed it can handle ten voices flawlessly (this is before filters and complex moduation)
    set_sys_clock_khz(270000, true); // 270 MHz 
    
    int midi_baud_rate = uart_init(uart1, 31250);
    gpio_set_function(5, GPIO_FUNC_UART); // 5 = midi pin
    uart_set_fifo_enabled(uart1, true);

    ///* maybe midi interrupt code? the uart MUST be initialized and set to the correct baud rate before
    //uart_set_irqs_enabled(uart1, true, false);
    uart_set_irq_enables(uart1, true, false);

    //irq_set_exclusive_handler(UART1_IRQ, on_uart_interrupt); // function doesn't exist 
    irq_set_exclusive_handler(UART1_IRQ, on_uart_interrupt);
    irq_set_enabled(UART1_IRQ, true);

    //*/

    gpio_init(ERR_LED_PIN);
    gpio_set_dir(ERR_LED_PIN, GPIO_OUT);
    
    gpio_init(MIDI_LED_PIN);
    gpio_set_dir(MIDI_LED_PIN, GPIO_OUT);

    adc_init();

    //adc_gpio_init(28);
    //adc_select_input(2);

    // all are initialized to be able to switch the input while running.
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);
    adc_select_input(1); // gpio 27

    // to mux input
    gpio_init(MUX_1_PIN);
    
    gpio_put(MUX_1_PIN, 1);
    
    gpio_init(MUX_2_PIN);
    gpio_set_dir(MUX_2_PIN, GPIO_OUT);
    gpio_put(MUX_2_PIN, 1);

    gpio_init(MUX_3_PIN);
    gpio_set_dir(MUX_3_PIN, GPIO_OUT);
    gpio_put(MUX_3_PIN, 1);

    int midi_counter = 0;
    uint8_t midi_cmd[] = {0, 0, 0};

    // problem fixed. caused by passing a factor instead of the time
    // XXX: the program is stopping at initialize_voice(). find out why
    // there is something in initialize_env() that causes the crash
    initialize_parameters(&global_paramaters);

    for (int i = 0; i < VOICE_COUNT; i++) {
        initialize_voice(&voices[i]);
    }

    initialize_controls();

    initialize_wavetable(sin_table, sin_wave);
    initialize_wavetable(square_table, square_wave);
    initialize_wavetable(sawtooth_table, sawtooth_wave);
    initialize_wavetable(sawtooth_table, triangle_wave);
    initialize_increment_table();

    multicore_launch_core1(core1_entry);

    gpio_put(ERR_LED_PIN, 1); // doesn't turn on, program doesn't get to here.

    while (1) {

        
        // mux circuit is incorrect, which results in random behavior.
        /*
        for (int i = 0; i <= 7; i++) { // from 0 to 8
            // 7 & 1 = 111 & 010 = 1
            //gpio_put(MUX_1_PIN, i & 1); // 1: (1 << 0)
            //gpio_put(MUX_2_PIN, (i & 2) >> 1); // 2 : (1 << 1)
            //gpio_put(MUX_3_PIN, (i & 4) >> 2); // 4: (1 << 2)

            gpio_put(MUX_1_PIN, 1); // 1: (1 << 0)
            gpio_put(MUX_2_PIN, 1); // 2 : (1 << 1)
            gpio_put(MUX_3_PIN, 1); // 4: (1 << 2)

            sleep_us(20);

            adc_read();
            int adc_input = adc_read();
            //float mod = adc_input * pot_divider;
            printf("%d: %d, ", i, adc_input);
            inputs[i] = adc_input;
        }
        printf("\n");
        */
    }
}
