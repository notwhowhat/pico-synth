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

#define VOICE_COUNT 16

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
volatile unsigned char midi_keys[128] = {0};
volatile unsigned char midi_previous_keys[128] = {0};

// the voice gets allocated a note, which it reads
struct voice {
    bool free;
    int note; // which note in midi_keys it's connected to
    waveform selected_waveform;
    float table_index;
};

// when set to zero EVERYTHING inside of the structs get too
volatile struct voice voices[VOICE_COUNT] = {0};

void initialize_voice(struct voice *v) {
    v->free = true;
    v->note = 0;
    v->selected_waveform = SAW;
    v->table_index = 0.0;
}

/* program flow for voice handler:
loop through new notes
loop through old notes
if new notes have shown up:
    loop through voices
    if any voices are free:
        set voice to not free
        set voice note
    else:
        loop through voices
        reset voice with lowest note 
        

loop through notes and save how many that are on
loop through voices and see how many that are free
if there are more notes than voices and
*/


/*
* PWM Interrupt Handler which outputs PWM level and advances the 
* current sample. 
* 
* We repeat the same value for 8 cycles this means sample rate etc
* adjust by factor of 8   (this is what bitshifting <<3 is doing)
* 
*/

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

/* program flow for voice handler:
loop through new notes
loop through old notes
if new notes have shown up:
    loop through voices
    if any voices are free:
        set voice to not free
        set voice note
    else:
        loop through voices
        reset voice with lowest note 
*/

// call it with isr (interrupt service routine)
void pwm_interrupt_handler() {
//    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    
//    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)0);
//}
//void stuff() {
    audio_counter++;
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    

    float master_out = 0.0;

    // i could use a linked list. but do i actually want to?
    int new_notes[VOICE_COUNT] = {0};
    int new_note_counter = 0;

    for (int i = 0; i < 128; i++) {
        // a new note!
        if (midi_keys[i] > 0 && midi_previous_keys[i] == 0) {

            // so that the synth doesn't have do redo the whole operation if someone plays way to many keys
            if (new_note_counter <= VOICE_COUNT) {
                new_notes[new_note_counter] = i;
                new_note_counter++;
            }
        }
    }

    struct voice *lowest_voice_note = voices;
    struct voice *free_voices[VOICE_COUNT];
    int free_voice_counter = 0;

    if (new_notes[0] != 0) {
        for (int i = 0; i < VOICE_COUNT; i++) {
            if (voices[i].free) {
                free_voices[free_voice_counter] = &voices[i];
                free_voice_counter++;
            } else {
                // the voice with the lowest note gets stolen
                if (voices[i].note < lowest_voice_note->note) {
                    lowest_voice_note = &voices[i];
                }
            }

        }
    }

    for (int i = 0; i < VOICE_COUNT; i++) {
        if (midi_keys[voices[i].note] != 0) {
            // this needs to be optimized by saving the increment later
            voices[i].table_index += (360 / (44100 / FREQUENCIES[i]));
            if (voices[i].table_index > 360) {
                voices[i].table_index -= 360;
            }

            master_out += oscillator(voices[i].selected_waveform, voices[i].table_index);
        } else {
            // envelopes are note available
            voices[i].free = true;
        }
    }
    
    waveform selected_waveform = SAW;

    //note_on = 0;
    //for (int i = 0; i < 128; i++) {
    //    if (midi_keys[i] != 0) {
    //        float frequency_ratio = 44100 / FREQUENCIES[i];
    //        sample_index += (360 / frequency_ratio);

    //        if (sample_index > 360) {
    //            sample_index -= 360;
    //        }
    //        note_on = 1;
    //    }
    //}
    //if (note_on == 1) {
    //    float sample = oscillator(selected_waveform, sample_index);
    //    master_out += sample;
    //}

    

    // basically this is mad.
    // it changes the pointer to the next element in p_table
    // and then gets the correct index.
    //uint16_t sample = 100 * 100 * (uint16_t)(*(p_table + (int)sample_index));
    //uint16_t sample = 100 + 100 * table[(int)sample_index];

    // this is done to make amplify the signal while making it unsigned in a controlled way
    // TODO: add a compressor like gain, so the volume gets set compared to the maximum size
    master_out = (master_out * gain) + gain;

    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)master_out);

    /*
    if (wav_position < (WAV_DATA_LENGTH<<3) - 1) { 
        // set pwm level 
        // allow the pwm value to repeat for 8 cycles this is >>3 
        //pwm_set_gpio_level(AUDIO_PIN, TABLE[get_table_index()]);  
        pwm_set_gpio_level(AUDIO_PIN, WAV_DATA[wav_position>>3]);  

        wav_position++;
        //wav_position += 100;
        //wav_position += 2;
    } else {
        // reset to start
        wav_position = 0;
    }
    */
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
    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_interrupt_handler); 
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
    unsigned char midi_cmd[] = {0, 0, 0};

    for (int i = 0; i < VOICE_COUNT; i++) {
        initialize_voice(&voices[i]);
    }

    

    while (1) {
        while (!uart_is_readable(uart0)) {
        }
        unsigned char midi_input = uart_getc(uart0);

        // must have data for to be parsed
        /*
        if (midi_input != 0) {
            switch (midi_counter) {
                case 0:
                    // status byte
                    midi_cmd[0] = midi_input;
                    midi_counter++;
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
        }
        */

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
            unsigned char status = midi_cmd[0] & 240;
            unsigned char channel = midi_cmd[1] & 15;

            switch (status) {
                case 128:
                    //gpio_put(ERR_LED_PIN, 0);
                    gpio_put(MIDI_LED_PIN, 0);
                    //note_on = 0;
                    //gain = 0;
                    midi_keys[midi_cmd[1]] = 0;
                    //printf("noff");
                    break;
                case 144:
                    //gpio_put(ERR_LED_PIN, 1);
                    gpio_put(MIDI_LED_PIN, 1);
                    //note_on = 1;

                    // this should work, but it's risky. check if it's a correct value
                    midi_keys[midi_cmd[1]] = midi_cmd[2];
                    //printf("non");
                    break;
            }

            midi_cmd[0] = 0;
            midi_cmd[1] = 0;
            midi_cmd[2] = 0;
        }
        // not anymore! __wfi(); // Wait for Interrupt
    }
}
