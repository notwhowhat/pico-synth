#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "pico/multicore.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/uart.h"
#include "hardware/timer.h"

#include "math.h"

// a permanent led pin for checking if any runtime errors have occured without a debugger
#define ERR_LED_PIN 25
#define MIDI_LED_PIN 15
#define AUDIO_PIN 2 

#define VOICE_COUNT 8

#include "wavetables.h"
#include "frequencies.h"


typedef enum {
    SIN,
    SAW,
    SQUARE,
} waveform;

int gain = 10;

// this fills it with zeros!
// two key arrays are needed to be ablet to distinguish between a new and old note for envel
uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

/*
envelopes will be y=kx+m
k = max amplitude / env part
a_mod = 1 / a_time
d_mod = - 1 / d_time
r_mod = - 1 / r_time
if time > a_time then mod * d_mod

envelope controll flow:
if note is off then do release
otherwise check if time < a_time then do attack
otherwise check if time < d_time then do release
otherwise do sustain
if it's off then reset time start release
then check if time

*/

/*
so what has to be done?
envelopes and lfo(s) for modulation
modulation for osc1 and osc2 frequency, amplitude, filter

implementing frequency modulation might be difficult because of the current oscillator design. 
currently, the table increment is only calculated once per note and it remains constant.
this has to be changed for frequency/pitch modulation to work 

oscilator aliasing is leaving huge artifacts on the sound. this can be fixed with filtering in the
generation of the wavetables.
i think i already have an algorithm for the filter. but i'm not sure about how well it will perform

it could be good to start working on the interface before i get to the modulation for real. then i can
actually try using the features, and see how they work when i make them so that i don't need to redo everything
because it doesn't work when the values change.
*/

struct env {
    int time;
    float mod;

    int a_time;
    int d_time;
    int r_time;
    
    float s_mod;
    float a_mod;
    float d_mod;
    float r_mod;
};

// the voice gets allocated a note, which it reads
struct voice {
    bool used;
    int note; // which note in midi_keys it's connected to
    int age;
    bool new;

    float table_index;
    float table_increment;

    waveform selected_waveform;

    struct env amp_env;
};

// when set to zero EVERYTHING inside of the structs get too
struct voice voices[VOICE_COUNT] = {0};

void initialize_env(struct env *e, int a_time, int d_time, int r_time, float s_mod) {
    e->time = 0;
    e->mod = 0.0;

    e->a_time = 20000;
    e->d_time = 20000;
    e->r_time = 20000;

    e->s_mod = 0.3;
    e->a_mod = 1.0 / e->a_time;
    e->d_mod = (1.0 - e->s_mod) / e->d_time;
    e->r_mod = e->s_mod / e->r_time;
}

void initialize_voice(struct voice *v) {
    v->used = false;
    v->note = 0;
    v->age = 0;
    v->new = false;

    v->table_index = 0.0;
    v->table_increment = 0.0;

    v->selected_waveform = SAW;

    initialize_env(&v->amp_env, 20000, 20000, 20000, 0.4);
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


float process_voice(struct voice *v) {
    if (v->used) {
        // it's off (0 = note off)
        if (midi_keys[v->note] == 0) {
            // the time only works if it gets reset for the release
            // it has to check for a new note-off
            if (midi_previous_keys[v->note] != 0) {
                // new command
                v->amp_env.time = 0;
                /// XXX DON"T DO THIS! I REALLY SHOULDN"T MESS WITH MIDI
                midi_previous_keys[v->note] = 0;
            }

            // check if it's releasing or if the note should be off 
            if (v->amp_env.time < v->amp_env.r_time) {
                // do release
                v->amp_env.mod -= v->amp_env.r_mod;
            } else {
                // reset voice
                initialize_voice(v);
            }
        } else {
            // the note is on!
            // check first if attack, then decay
            if (v->amp_env.time < v->amp_env.a_time) {
                // do attack
                v->amp_env.mod += v->amp_env.a_mod;
            } else if (v->amp_env.time < (v->amp_env.a_time + v->amp_env.d_time)) {
                // do decay
                // the decay is starting from where the attack is, but it's still going up
                v->amp_env.mod -= v->amp_env.d_mod;
            } else {
                // do sustain 
                v->amp_env.mod = v->amp_env.s_mod;
            }
        }
        // time starts counting up since the start. the envelope has already gone through the attack and decay. (not the problem because of poor code)
        // the wave also goes out of phase between the envelope period changes. this is probably because the wave is made negative. (this is what causes the click)
        // so i need to do it in a way without inverting the phase
        // this could maybe be done by making the mod negative, and then taking it 1-mod or something similar
        //if (voices[i].used) {
        v->amp_env.time++;
    }
    
    
    // this is where the notes get stuck playing
    //if (midi_keys[voices[i].note] != 0) {
    if (v->amp_env.mod != 0.0) {
        // this could technicaly be used for performance, but it might be causing the vibrato chord bug
        if (v->table_increment == 0.0) {
            // this maybe optimizes
            //v->table_increment = (360 / (44100 / FREQUENCIES[v->note]));
            v->table_increment = (360 / (44100 / FREQUENCIES[50 + 100 * v->note]));
        }

        v->table_index += v->table_increment;
        //voices[i].table_index += (360 / (44100 / FREQUENCIES[voices[i].note]));
        if (v->table_index > 360.0) {
            v->table_index = v->table_index - 360.0;
        }

        // the gain must be set to something sensible, otherwise the it get's too loud, so the int's
        // sometimes get overloaded and an lfo-like sound occurs.
        //master_out += voices[i].mod * oscillator(voices[i].selected_waveform, voices[i].table_index);
        return v->amp_env.mod * oscillator(v->selected_waveform, v->table_index);
    }
    return 0.0;
}

// call it with isr (interrupt service routine)
void on_pwm_interrupt() {
    //uint32_t start = time_us_32();
    pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    

    float master_out = 0.0;

    //for (struct voice *v = voices; v < voices + VOICE_COUNT; v++) {
    for (int i = 0; i < VOICE_COUNT; i++) {
        struct voice *v = &voices[i];
        master_out += process_voice(v);
    }
    
    master_out = gain * (master_out + gain);

    pwm_set_gpio_level(AUDIO_PIN, (uint16_t)master_out);
    //uint32_t d_time = time_us_32() - start;
    //if (d_time > slowest_time) {
    //    slowest_time = d_time;
    //    printf("%d\n", slowest_time);
    //}
}

/*
priority for main program loop:

create sample from previous values if there are any
set sample to global one to be fed to the audio hardware 
(so that if it doesn't finish making a sample it doesn't output something crazy)

read in and set midi values
read in and set parameters

this seems good because it should let the audio process as much as it needs.


i think the next step is to fix modulation. and stop the oscillators from aliasing, but it 
could be better to do that in combination with the filters. i don't know

the next important step for modulation is to be able to alter the pitch on the fly, hopefuly in semitones.
this opens up the possibility for both simple modulation with envelopes and lfo's, but also supersaws!
*/

void process_midi_commands(uint8_t cmd_fn, uint8_t note, uint8_t velocity) {
    switch(cmd_fn) {
        case 128:
            gpio_put(MIDI_LED_PIN, 0);
            printf("off @ %d\n", note);
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = 0;
            break;
        case 144:
            gpio_put(MIDI_LED_PIN, 1);
            //printf("on @ %d\n", midi_cmd[1]);

            // this should work, but it's risky. check if it's a correct value
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = velocity;
            
            // so that the synth doesn't have do redo the whole operation if someone plays way to many keys

            // the voice stealing is now done so that the lowest note is stolen.
            struct voice *selected_voice = voices; // &voices[0], because array's are mad

            for (int i = 0; i < VOICE_COUNT; i++) {
                if (!voices[i].used) {
                    selected_voice = &voices[i];
                    break;
                } else {
                    // the voice with the lowest note gets stolen i don't know if this is good. 
                    // i could probably change it if i have timers, but what would happen is that
                    // the variables would get too long if they would be timed in ms.
                    if (voices[i].age < selected_voice->age) {
                        selected_voice = &voices[i];
                    }
                    //if (voices[i].note < selected_voice->note) {
                    //    selected_voice = &voices[i];
                    //}
                }
                voices[i].age++;
            }
            
            selected_voice->note = note;
            selected_voice->used = true;
            selected_voice->new = true;
            break;
        case 176: // cc
            if (note == 1) {
                // modwheel
                //printf("%d", velocity);
            }
            break;
        case 224: // pitch bend
            break;
    }
}

void process_midi(void) {
    static int midi_counter = 0;
    static uint8_t midi_cmd[] = {0, 0, 0};
    static uint8_t cmd_fn = 0;
    static uint8_t cmd_channel = 0;

    // it's actually waiting here quite a bit
    while (!uart_is_readable(uart1)) {
    }

    uint8_t midi_input = uart_getc(uart1);
    //printf("%d\n", midi_input);

    midi_cmd[midi_counter] = midi_input;
    midi_counter++;

    // this can probably optimized a bit!
    // there will only be one new note. so i can remove everything with that.

    // it doesn't send the status byte if it was the same as the last
    // that's the problem! I FOUUUUNNND IT!
    // the status byte always starts with a zero, so it's always bigger than 127!!

    if (midi_counter == 2 && midi_cmd[0] < 128) {
        // this is technicaly unsafe, but come on.
        if (cmd_fn != 0) { // uninitialized
            //printf("short\n");
            process_midi_commands(cmd_fn, midi_cmd[0], midi_cmd[1]);
        }
        
        // process midi with last status byte
        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        midi_counter = 0;

    } else if (midi_counter == 3) {
        //printf("new\n");
        cmd_fn = midi_cmd[0] & 240;
        cmd_channel = midi_cmd[0] & 15;
        process_midi_commands(cmd_fn, midi_cmd[1], midi_cmd[2]);

        midi_cmd[0] = 0;
        midi_cmd[1] = 0;
        midi_cmd[2] = 0;
        midi_counter = 0;
    }
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
    pwm_config_set_wrap(&config, 437); 
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
    //set_sys_clock_khz(124000, true);
    // at the higher clock speed it can handle ten voices flawlessly (this is before filters and complex moduation)
    set_sys_clock_khz(270000, true);
    
    int midi_baud_rate = uart_init(uart1, 31250);
    gpio_set_function(5, GPIO_FUNC_UART);
    uart_set_fifo_enabled(uart1, true);

    //core1_entry();

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
