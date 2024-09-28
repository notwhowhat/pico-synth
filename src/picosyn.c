#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "pico/multicore.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/uart.h"

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
uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

// the voice gets allocated a note, which it reads
struct voice {
    bool used;
    int note; // which note in midi_keys it's connected to
    waveform selected_waveform;
    float table_index;
    float table_increment;
};

// when set to zero EVERYTHING inside of the structs get too
volatile struct voice voices[VOICE_COUNT] = {0};

void initialize_voice(volatile struct voice *v) {
    v->used = false;
    v->note = 60;
    v->selected_waveform = SAW;
    v->table_index = 0.0;
    v->table_increment = 0.0;
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
void on_pwm_interrupt() {
    audio_counter++;
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    

    float master_out = 0.0;
    // i forgot to add notes to free voices ;)

    // TODO: i should actually use static for all of the voices and define them here. i know
    // that it shouldn't work, but it doesn. i don't really know why so. if they're static they
    // will only be accessed in here, and they can be initialized at decleration, without being overwritten.
    // i can use static for voices tooo!!!! i might be happy

    for (int i = 0; i < VOICE_COUNT; i++) {
        // potential bug
        
        // this is where the notes get stuck playing
        if (midi_keys[voices[i].note] != 0) {
            // not off
            // performance problem
            if (voices[i].table_increment == 0.0) {
                // this maybe optimizes
                voices[i].table_increment = (360 / (44100 / FREQUENCIES[voices[i].note]));
            }
            //voices[i].table_index += (360 / (44100 / FREQUENCIES[voices[i].note]));
            //voices[i].table_index += ((44100 / FREQUENCIES[i]) / 360);
            voices[i].table_index += voices[i].table_increment;
            if (voices[i].table_index > 360.0F) {
                voices[i].table_index = voices[i].table_index - 360.0F;
            }

            master_out += oscillator(voices[i].selected_waveform, voices[i].table_index);
        } else {
            // envelopes are note available
            voices[i].used = false;
            voices[i].table_increment = 0.0;
        }
    }
    
    //waveform selected_waveform = SIN;

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

*/

void process_midi(void) {
    static int midi_counter = 0;
    static uint8_t midi_cmd[] = {0, 0, 0};
    static uint8_t cmd_fn = 0;

    // it's actually waiting here quite a bit
    while (!uart_is_readable(uart1)) {
    }

    uint8_t midi_input = uart_getc(uart1);
    printf("%d\n", midi_input);

    midi_cmd[midi_counter] = midi_input;
    midi_counter++;

    // this can probably optimized a bit!
    // there will only be one new note. so i can remove everything with that.

    // it doesn't send the status byte if it was the same as the last
    // that's the problem! I FOUUUUNNND IT!
    // the status byte always starts with a zero, so it's always bigger than 127!!

    // MAKE THIS INTO A FUNCTION BEFORE I GO INSANE
    // also add voice suppoooorrt
    if (midi_counter == 2 && midi_cmd[0] < 128) {
        // this is technicaly unsafe, but come on.
        if (cmd_fn != 0) { // uninitialized
            printf("short\n");
            switch (cmd_fn) {
                case 128:
                    gpio_put(MIDI_LED_PIN, 0);
                    printf("off @ %d\n", midi_cmd[0]);
                    midi_previous_keys[midi_cmd[0]] = midi_keys[midi_cmd[0]];
                    midi_keys[midi_cmd[0]] = 0;
                    break;
                
                case 144: // note on
                    gpio_put(MIDI_LED_PIN, 1);
                    printf("on @ %d\n", midi_cmd[0]);

                    // this should work, but it's risky. check if it's a correct value
                    midi_previous_keys[midi_cmd[0]] = midi_keys[midi_cmd[0]];
                    midi_keys[midi_cmd[0]] = midi_cmd[1];
                    
                    // so that the synth doesn't have do redo the whole operation if someone plays way to many keys

                    struct voice *selected_voice = voices; // &voices[0], because array's are mad

                    for (int i = 0; i < VOICE_COUNT; i++) {
                        if (!voices[i].used) {
                            selected_voice = &voices[i];
                            break;
                        } else {
                            // the voice with the lowest note gets stolen i don't know if this is good. 
                            // i could probably change it if i have timers, but what would happen is that
                            // the variables would get too long if they would be timed in ms.
                            if (voices[i].note < selected_voice->note) {
                                selected_voice = &voices[i];
                            }
                        }
                    }
                    
                    selected_voice->note = midi_cmd[0];
                    selected_voice->used = true;
                    break;
            }
        }
        
        // process midi with last status byte
        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        midi_counter = 0;

    } else if (midi_counter == 3) {
        printf("new\n");
        cmd_fn = midi_cmd[0] & 240;
        switch(cmd_fn) {
            case 128: // note off
                gpio_put(MIDI_LED_PIN, 0);
                printf("off @ %d\n", midi_cmd[1]);
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = 0;
                break;
            case 144: // note on
                gpio_put(MIDI_LED_PIN, 1);
                printf("on @ %d\n", midi_cmd[1]);

                // this should work, but it's risky. check if it's a correct value
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = midi_cmd[2];
                
                // so that the synth doesn't have do redo the whole operation if someone plays way to many keys

                struct voice *selected_voice = voices; // &voices[0], because array's are mad

                for (int i = 0; i < VOICE_COUNT; i++) {
                    if (!voices[i].used) {
                        selected_voice = &voices[i];
                        break;
                    } else {
                        // the voice with the lowest note gets stolen i don't know if this is good. 
                        // i could probably change it if i have timers, but what would happen is that
                        // the variables would get too long if they would be timed in ms.
                        if (voices[i].note < selected_voice->note) {
                            selected_voice = &voices[i];
                        }
                    }
                }
                
                selected_voice->note = midi_cmd[1];
                selected_voice->used = true;
                break;
        }


        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        midi_counter = 0;
    }

    /*
    if (midi_counter == 3) {
        midi_counter = 0;

        uint8_t status = midi_cmd[0] & 240;
        uint8_t channel = midi_cmd[1] & 15;

        // this can probably optimized a bit!
        // there will only be one new note. so i can remove everything with that.

        // it doesn't send the status byte if it was the same as the last
        // that's the problem! I FOUUUUNNND IT!
        // the status byte always starts with a zero, so it's always bigger than 127!!

        switch (status) {
            case 128: // note off
                gpio_put(MIDI_LED_PIN, 0);
                printf("off @ %d\n", midi_cmd[1]);
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = 0;
                break;
            case 144: // note on
                gpio_put(MIDI_LED_PIN, 1);
                printf("on @ %d\n", midi_cmd[1]);

                // this should work, but it's risky. check if it's a correct value
                midi_previous_keys[midi_cmd[1]] = midi_keys[midi_cmd[1]];
                midi_keys[midi_cmd[1]] = midi_cmd[2];
                
                // so that the synth doesn't have do redo the whole operation if someone plays way to many keys

                struct voice *selected_voice = voices; // &voices[0], because array's are mad

                for (int i = 0; i < VOICE_COUNT; i++) {
                    if (!voices[i].used) {
                        selected_voice = &voices[i];
                        break;
                    } else {
                        // the voice with the lowest note gets stolen i don't know if this is good. 
                        // i could probably change it if i have timers, but what would happen is that
                        // the variables would get too long if they would be timed in ms.
                        if (voices[i].note < selected_voice->note) {
                            selected_voice = &voices[i];
                        }
                    }
                }
                
                selected_voice->note = midi_cmd[1];
                selected_voice->used = true;
                break;
        }

        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        }
*/
}

/* 
time to implement multithreading! it will be split up in two threads. the load
won't be balanced in any way, instead, the midi will be on one core, and the audio processing will be on the
other one. this will probably be the best solution because the audio is triggered by an isr.

i will share the voices in global memory. it's suboptimal, but i will at least be using mutexes instead.
where should i the audio get processed? probably in the second core? idk.
*/

void core1_entry() {
    gpio_set_function(AUDIO_PIN, GPIO_FUNC_PWM);

    int audio_pin_slice = pwm_gpio_to_slice_num(AUDIO_PIN);

    pwm_clear_irq(audio_pin_slice);
    pwm_set_irq_enabled(audio_pin_slice, true);
    // set the handle function above
    irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_interrupt); 
    irq_set_enabled(PWM_IRQ_WRAP, true);

    // https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga8478ee26cc144e947ccd75b0169059a6

    // Setup PWM for audio output
    pwm_config config = pwm_get_default_config();
    // these values are set to run at the sample rate of 44.1KHz.
    pwm_config_set_clkdiv(&config, 14.0f); 
    pwm_config_set_wrap(&config, 200); 
    pwm_init(audio_pin_slice, &config, true);

    pwm_set_gpio_level(AUDIO_PIN, 0);

    while (1) {
        tight_loop_contents();
    }
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
    set_sys_clock_khz(124000, true);
    
    int midi_baud_rate = uart_init(uart1, 31250);
    gpio_set_function(5, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart1, true);

    //core1_entry();

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

    multicore_launch_core1(core1_entry);

    while (1) {
        process_midi();
    }
}
