#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 

#include "math.h"

// a permanent led pin for checking if any runtime errors have occured without a debugger
#define ERR_LED_PIN 25
#define MIDI_LED_PIN 15
#define AUDIO_PIN 2 

#define VOICE_COUNT 5

#include "wavetables.h"
#include "frequencies.h"

int available_available = 16;

int audio_counter = 0;
float sample_index = 0.0;

typedef enum {
    SIN,
    SAW,
    SQUARE,
} waveform;

int gain = 100;
volatile int note_on = 0;

// this fills it with zeros!
// two key arrays are needed to be ablet to distinguish between a new and old note for envel
uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

// the voice gets allocated a note, which it reads
struct voice {
    bool available;
    int note; // which note in midi_keys it's connected to
    waveform selected_waveform;
    float table_index;
};

// when set to zero EVERYTHING inside of the structs get too
volatile struct voice voices[VOICE_COUNT] = {0};

void initialize_voice(volatile struct voice *v) {
    v->available = true;
    v->note = 0;
    v->selected_waveform = SIN;
    v->table_index = 0.0;
}

float oscillator(waveform selected_waveform, float sample_index) {
    // sample_index is still passed as a float for other interpolation methods later
    switch (selected_waveform) {
        case SIN:
            return SIN_TABLE[(int)sample_index];
            break;
       case SAW:
            return SAW_TABLE[(int)sample_index];
            break;
       case SQUARE:
            return SQUARE_TABLE[(int)sample_index];
            break;
        default:
            return 0.0;
    }
}

// call it with isr (interrupt service routine)
void on_pwm_interrupt() {
    audio_counter++;
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    

    float master_out = 0.0;

    // all of the audio processing is done here
    for (int i = 0; i < VOICE_COUNT; i++) {
        // potential bug
        
        // this is where the notes get stuck playing
        if (midi_keys[voices[i].note] != 0) {
            // not off
            // performance problem
            //voices[i].table_index += (360 / (44100 / FREQUENCIES[i]));
            //if (voices[i].table_index > 360.0F) {
            //    voices[i].table_index = voices[i].table_index - 360.0F;
            //}

            //master_out += oscillator(voices[i].selected_waveform, voices[i].table_index);
        } else {
            // envelopes are note available
            voices[i].available = true;
        }
    }
    
    master_out = (master_out * gain) + gain;

    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)master_out);
}

int process_audio(void) {
    // all audio and midi processing should be done here, instead of in the interurpt
    // or not. idk

    return 0;
}

/*
priority for main program loop:

create sample from previous values if there are any
set sample to global one to be fed to the audio hardware 
(so that if it doesn't finish making a sample it doesn't output something crazy)

read in and set midi values
read in and set parameters

this seems good because it should let the audio process as much as it needs.
it is actually good if the sample is generated in the isr, so that it's always updated.
then it should always work well if i don't use some crazy fft

i should probably use interpolation/averaging the filter when the parameters are changed.

*/

void process_midi(void) {
    static int midi_counter = 0;
    static uint8_t midi_cmd[] = {0, 0, 0};

    while (!uart_is_readable(uart0)) {
    }
    uint8_t midi_input = uart_getc(uart0);

    switch (midi_counter) {
        case 0:
            if (midi_input != 0) {
                // this should have fixed most of the problems
                // what i think was happening before is that it was trying to 
                // recieve a note off using a note on cc 
                // status byte
                midi_cmd[0] = midi_input;
                midi_counter++;
            }
            break;
        case 1:
            midi_cmd[1] = midi_input;
            midi_counter++;
            break;
        case 2:
            midi_cmd[2] = midi_input;
            midi_counter++;
            break;
    }

    // this might result in a bug if the velocity is zero (note off)
    // TODO: i think that this might be the bug
    // i can change the use of midi_counter
    //if (midi_cmd[2] != 0) {

    // this midi_counter==3 code shouldn't be needed with the new code added
    // i'll keep it temporarily so that i don't break anything more
    if (midi_counter == 3) {
        midi_counter = 0;

        uint8_t status = midi_cmd[0] & 240;
        uint8_t channel = midi_cmd[1] & 15;

        // i could use a linked list. but do i actually want to?
        int new_notes[VOICE_COUNT] = {0};
        int new_note_counter = 0;

        switch (status) {
            case 128: // note off
                gpio_put(MIDI_LED_PIN, 0);
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = 0;
                break;
            case 144: // note on
                // refactor this complete dumpsterfire

                gpio_put(MIDI_LED_PIN, 1);
                // this should work, but it's risky. check if it's a correct value
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = midi_cmd[2];
                
                // so that the synth doesn't have do redo the whole operation if someone plays way to many keys
                if (new_note_counter < VOICE_COUNT) {
                    new_notes[new_note_counter] = midi_cmd[1];
                    new_note_counter++;
                }

                struct voice *lowest_voice_note = voices;
                struct voice *available_voices[VOICE_COUNT];
                int available_voice_counter = 0;

                // gets available voices and steals the lowest voice
                if (new_notes[0] != 0) {
                    for (int i = 0; i < VOICE_COUNT; i++) {
                        if (voices[i].available) {
                            available_voices[available_voice_counter] = &voices[i];
                            available_voice_counter++;
                        } else {
                            // the voice with the lowest note gets stolen
                            if (voices[i].note < lowest_voice_note->note) {
                                // technically it should sort all of the voices and put them in 
                                // order, but i don't have time for that
                                lowest_voice_note = &voices[i];
                                // 8 levels of indentation
                            }
                        }
                    }
                    
                    // goes through the available voices and sets the new notes to them
                    // this is a wip. it shouldn't and doesn't work
                    // i actually think that this is intentional.
                    for (int i = 0; i < new_note_counter; i++) {
                        available_voices[i]->note = new_notes[i];
                        available_voices[i]->available = false;
                    }
                }
                break;
        }

        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        }
}

int main(void) {
    /* Overclocking for fun but then also so the system clock is a 
     * multiple of typical audio sampling rates.
     */
    stdio_init_all();

    // TODO: its the clock frequency that's wrong. at just a tiny bit more it works flawelessly, but 
    // if i just lower it a tiny bit it errors, because of no led output. 
    //set_sys_clock_khz(123480, true); 
    set_sys_clock_khz(124000, true); 
    
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);
    

    // Setup PWM interrupt to fire when PWM cycle is complete
    // TODO XXX: this should maybe be set last, so that an interrupt doesn't trigger before everything's initialized
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_interrupt); 
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // for the midi irq handelling
    // uart_set_irq_enables()
    // irq_set_exclusive_handler(UART0_IRQ, midi handler)
    // i should use UART0_IRQ or UART1_IRQ as the sources
    // https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga8478ee26cc144e947ccd75b0169059a6
    // ill just have midi in the loop instead :>

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 14.0f); 
    pwm_config_set_wrap(&config, 200); 
    pwm_init(audio_pin_slice, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    int baud_rate = uart_init(uart0, 31250);
    gpio_set_function(1, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart0, true);

    // backup sin generator
    //float increment = 2.0 * M_PI / 360;
    //for (int i = 0; i < 360; i++) {
    //    //uint16_t sample = 100 + 100 * sin((audio_counter / div) * (2 * M_PI / 100));
    //    sin_table[i] = (float)sin(increment * i); 
    //}

    gpio_init(ERR_LED_PIN);
    gpio_set_dir(ERR_LED_PIN, GPIO_OUT);
    gpio_put(ERR_LED_PIN, 1);
    
    gpio_init(MIDI_LED_PIN);
    gpio_set_dir(MIDI_LED_PIN, GPIO_OUT);

    int midi_counter = 0;
    uint8_t midi_cmd[] = {0, 0, 0};

    for (int i = 0; i < VOICE_COUNT; i++) {
        initialize_voice(&voices[i]);
    }

    while (1) {
        process_midi();
    }
}
