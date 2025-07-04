#include <stdio.h>
#include "pico/stdlib.h"   // stdlib 
#include "pico/multicore.h"
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm 
#include "hardware/sync.h" // wait for interrupt 
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#include "math.h"

// a permanent led pin for checking if any runtime errors have occured without a debugger
#define ERR_LED_PIN 25
#define MIDI_LED_PIN 15
#define AUDIO_PIN 2 

#define MUX_1_PIN 18
#define MUX_2_PIN 19
#define MUX_3_PIN 20 

#define VOICE_COUNT 8

#include "tables.h"

#define LOWPASS 0
#define HIGHPASS 1

float controls[8];

typedef enum {
    SIN,
    SAW,
    SQUARE,
} waveform;

typedef enum {
    A, D, S, R,
} env_state;

int gain = 10;

// this fills it with zeros!
// two key arrays are needed to be ablet to distinguish between a new and old note for envel
uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

/*
paramaters will be done so that they are stored in a struct or array. there will also be a second
one holding the old values. every struct element will be updated and moved simultaniously.
*/
const float pot_divider = 1.0 / 4095.0;
float pot_mod = 1.0;

const int max_detune[7] = {
    -191, -109, -37,
    0,
    31, 107, 181,
};

// paramaters only to be used by core 1 (inputs)

//float filter_cutoff = 1.0;
//float filter_res = 0.0;

//float cutoff = 0.1;
//float resonance = 0.2;
//
//float filter1 = 0.0, filter2 = 0.0, filter3 = 0.0, filter4 = 0.0;

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

/* 
new idea for enveloes:
enum containing state.
if mod > state cap then enter next state (could be done with enum)
*/

struct env {
    int time;
    float mod;
    env_state state;

    int a_time;
    int d_time;
    int r_time;
    
    float s_mod;
    float a_mod;
    float d_mod;
    float r_mod;
};

struct osc {
    float table_index;
    float table_increment;
    waveform selected_waveform;
};

struct filter {
    float cutoff;
    float resonance;
    bool type;

    // buffers for orders of filter
    float first;
    float second;
    float third;
    float fourth;
};

// the voice gets allocated a note, which it reads
struct voice {
    bool used;
    int note; // which note in midi_keys it's connected to
    int age;
    bool new;

    float table_index;
    float table_increment;

    struct osc osc1;

    //struct osc oscillators[7];

    waveform selected_waveform;

    struct env amp_env;
    struct env filter_env;

    struct filter lowpass;
};

// when set to zero EVERYTHING inside of the structs get too
struct voice voices[VOICE_COUNT] = {0};

void initialize_osc(struct osc *osc, waveform selected_waveform) {
    //osc->table_index = 0.0;

    osc->table_increment = 0.0;
    osc->selected_waveform = selected_waveform;
}

float process_osc(struct osc *osc, int note_increment) {
    osc->table_index += INCREMENT_TABLE[50 + note_increment];
    if (osc->table_index > 360.0) {
        osc->table_index = osc->table_index - 360.0;
    }
    
    switch (osc->selected_waveform) {
        case SIN:
            return SIN_TABLE[(int)osc->table_index];
            break;
       case SAW:
            return SAW_TABLE[(int)osc->table_index];
            break;
       case SQUARE:
            return SQUARE_TABLE[(int)osc->table_index];
            break;
        default:
            return 0.0;
    }
}

void initialize_filter(struct filter *f, float cutoff, float resonance, bool type) {
    f->cutoff = cutoff;
    f->resonance = resonance;
    f->type = type;

    f->first = 0.0;
    f->second = 0.0;
    f->third = 0.0;
    f->fourth = 0.0;
}

float process_lowpass(struct filter *f, float input) {
    f->first += ((input - (f->fourth * f->resonance)) - f->first) * f->cutoff;
    f->second += (f->first - f->second) * f->cutoff;
    f->third += (f->second - f->third) * f->cutoff;
    f->fourth += (f->third - f->fourth) * f->cutoff;
    
    return f->fourth;
}

float process_filter(struct filter *f, float input, float mod) {
    // to acieve a highpass filter, remove the lowpass from the input.
    // highpass = input - lowpass


    // it becomes infinite

    if (f->type != LOWPASS) {
        f->cutoff = (1.0 - pot_mod);// * mod;
        return input - process_lowpass(f, input);
    } else {
        f->cutoff = pot_mod * mod;
        return process_lowpass(f, input);
    }
    
}

// the minimum time: one sample
void initialize_env(struct env *e, int a_time, int d_time, int r_time, float s_mod) {
    e->time = 0;
    e->mod = 0.0;
    e->state = A;

    e->a_time = a_time;
    e->d_time = d_time;
    e->r_time = r_time;

    e->s_mod = s_mod;
    e->a_mod = 1.0 / e->a_time;
    e->d_mod = (1.0 - e->s_mod) / e->d_time;
    e->r_mod = e->s_mod / e->r_time;
}

void update_env(struct env *e, int a_time, int d_time, int r_time, float s_mod) {
    switch (e->state) {
        case A:
            e->a_time = a_time;
            e->a_mod = 1.0 / e->a_time;
        case D:
            e->d_time = d_time;
            e->d_mod = (1.0 - e->s_mod) / e->d_time;
        //case S:
        //    e->s_mod = s_mod;
        case R:
            e->r_time = r_time;
            e->r_mod = e->s_mod / e->r_time;
    }
}

void initialize_voice(struct voice *v) {
    v->used = false;
    v->note = 0;
    v->age = 0;
    v->new = false;

    v->table_index = 0.0;
    v->table_increment = 0.0;

    v->selected_waveform = SAW;

    initialize_env(&v->amp_env, 1, 1, 1, 1.0);
    //initialize_env(&v->filter_env, 1, 5000, 1, 1.0);

    initialize_osc(&v->osc1, SAW);
    // the supersaw sound like a lazer because the phases are the same in the beginning, 
    // which makes them sound louder and out of tune.
    // for the not-very-super saw
    //for (int i = 1; i < 7; i++) {
    //    initialize_osc(&v->oscillators[i], SAW);
    //}

    //initialize_filter(&v->lowpass, 1.0, 0.0, LOWPASS);
}

void start_env_r(struct env *e) {
    e->time = 0;
    e->state = R;
}

void process_env_r(struct env *e, bool amp) {
    if (e->time < e->r_time) {
        // do release
        e->mod -= e->r_mod;
    } else {
        // reset voice
        e->mod = 0.0;
    }

    e->time++;
}

void process_env_ads(struct env *e) {
    if (e->time < e->a_time) {
        // do attack
        e->mod += e->a_mod;
        e->state = A;
    } else if (e->time < (e->a_time + e->d_time)) {
        // do decay
        // the decay is starting from where the attack is, but it's still going up
        e->mod -= e->d_mod;
        e->state = D;
    } else {
        // do sustain 
        e->mod = e->s_mod;
        e->state = S;
    }

    e->time++;
}

// what can happen when the envelopes get reset is that the time wrong.

// remove and make into two functions
float process_voice(struct voice *v) {
    if (v->used) {
        if (midi_keys[v->note] == 0) {
            if (midi_previous_keys[v->note] != 0) {
                // do this with all envelopes
                start_env_r(&v->amp_env);
                //start_env_r(&v->filter_env);

                midi_previous_keys[v->note] = 0;
            }

            process_env_r(&v->amp_env, true);
            //process_env_r(&v->filter_env, true);

            if (v->amp_env.mod == 0.0) {
                initialize_voice(v);
            }
        } else {
            // do this with all enveloeps
            process_env_ads(&v->amp_env);
            //process_env_ads(&v->filter_env);
        }

        //v->amp_env.time++;
    }
    
    // this is where the notes get stuck playing
    //if (midi_keys[voices[i].note] != 0) {
    if (v->amp_env.mod != 0.0) {

        //v->table_index += INCREMENTS[50 + 100 * v->note];
        //if (v->table_index > 360.0) {
        //    v->table_index = v->table_index - 360.0;
        //}

        //// the gain must be set to something sensible, otherwise the it get's too loud, so the int's
        ////master_out += voices[i].mod * oscillator(voices[i].selected_waveform, voices[i].table_index);
        //return v->amp_env.mod * oscillator(v->selected_waveform, v->table_index);
        float out = process_osc(&v->osc1, v->note * 100);

        // bad supersaw. it's really simple. no pitch tracking allpass filters, just a bit of detuning.
        // sounds nothing like the original
        //float out = 0.0;
        //for (int i = 0; i < 7; i++) {
        //    out += process_osc(&v->oscillators[i], v->note * 100 + max_detune[i] * pot_mod);//0.25);
        //}

        //out = process_filter(&v->lowpass, out, v->filter_env.mod);
        out *= v->amp_env.mod;

        return out;
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

    // THE FILTER ACTUALLY WOOORRRKKKSSS!!!
    // (i haven't tried resonance, but the cutoff works for sure. (kindof))

    // to acieve a highpass filter, remove the lowpass from the input.
    // highpass = input - lowpass

    //resonance = pot_mod;
    //float input = master_out - (filter4 * resonance);

    ////cutoff = pot_mod;
    
    //filter1 += (input - filter1) * cutoff;
    //filter2 += (filter1 - filter2) * cutoff;
    //filter3 += (filter2 - filter3) * cutoff;
    //filter4 += (filter3 - filter4) * cutoff;

    //master_out = filter4;
    
    //master_out = gain * pot_mod * (master_out + gain * pot_mod);
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
    // XXX: different devices output midi note off's in different ways. ableton and my midi keyboard send a ntoe off 
    // command, whilst the mininova just sends a note on with a velocity of zero. both work, but the outputs are set up
    // for the first alternative for performence reasons.
    // TODO: voice stealing doesn't really work. it frees the voice to be stolen, but it doesn't give it a note
    switch(cmd_fn) {
        case 128:
            gpio_put(MIDI_LED_PIN, 0);
            //printf("off @ %d\n", note);
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = 0;
            break;
        case 144:
            gpio_put(MIDI_LED_PIN, 1);
            //printf("on @ %d\n", note);

            // this should work, but it's risky. check if it's a correct value
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = velocity;
            
            struct voice *selected_voice = voices; // &voices[0], because array's are mad

            for (int i = 0; i < VOICE_COUNT; i++) {
                if (!voices[i].used) {
                    selected_voice = &voices[i];
                    break;
                } else {
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

void on_uart_interrupt(void) {
    static int midi_counter = 0;
    static uint8_t midi_cmd[] = {0, 0, 0};
    static uint8_t cmd_fn = 0;
    static uint8_t cmd_channel = 0;

    // it's actually waiting here quite a bit
    //while (!uart_is_readable(uart1)) {
    //}
    if (uart_is_readable(uart1)) {
        uint8_t midi_input = uart_getc(uart1);
        //printf("%d\n", midi_input);

        midi_cmd[midi_counter] = midi_input;
        midi_counter++;

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
}

//void on_uart_interrupt(void) {
//    process_midi();
//}

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

void core1_entry() {
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
    set_sys_clock_khz(270000, true);
    
    int midi_baud_rate = uart_init(uart1, 31250);
    gpio_set_function(5, GPIO_FUNC_UART); // 5 = midi pin
    uart_set_fifo_enabled(uart1, true);

    ///* maybe midi interrupt code? the uart MUST be initialized and set to the correct baud rate before
    //uart_set_irqs_enabled(uart1, true, false);
    uart_set_irq_enables(uart1, true, false);

    irq_set_exclusive_handler(UART1_IRQ, on_uart_interrupt); // function doesn't exist 
    irq_set_enabled(UART1_IRQ, true);

    //*/

    gpio_init(ERR_LED_PIN);
    gpio_set_dir(ERR_LED_PIN, GPIO_OUT);
    gpio_put(ERR_LED_PIN, 1);
    
    gpio_init(MIDI_LED_PIN);
    gpio_set_dir(MIDI_LED_PIN, GPIO_OUT);

    adc_init();

    //adc_gpio_init(28);
    //adc_select_input(2);

    adc_gpio_init(27);
    adc_select_input(1);

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


    for (int i = 0; i < VOICE_COUNT; i++) {
        initialize_voice(&voices[i]);
    }

    multicore_launch_core1(core1_entry);

    while (1) {
        // unfortunately the midi is blocking, which does so that it doesn't read it constantly.
        // a much safer and more reliable way would be to set up an interrupt for the midi so that it becomes prioritized.
        // i'm not sure if the midi code is quick enough, but it has to work. another way of making the midi code more reliable
        // is to buffer it, but i don't think it's very needed. it probably slows the code down more than it helps.
        //process_midi();

        // with this configuration of the cores, it might be better to compute and set the controls in core 0.
        // even though this idea could be good, it will create huge problems related to concurrency.

        float inputs[8] = {-1.0};

        for (int i = 0; i <= 7; i++) { // from 0 to 8
            gpio_put(MUX_1_PIN, i);
            gpio_put(MUX_2_PIN, i>>1);
            gpio_put(MUX_3_PIN, i>>2);

            int adc_input = adc_read();
            inputs[i] = adc_input * pot_divider;
            if (i == 0) {
                //printf("%d: %d\n", i, adc_input);
                pot_mod = adc_input * pot_divider;
            }
            printf("%d: %d\n", 2, inputs[2]);
        }

        for (int i = 0; i < VOICE_COUNT; i++) {
            update_env(&voices[i].amp_env, 80000 * inputs[0], 80000 * inputs[1], 80000 * inputs[2], inputs[3]);
        }


        //debug_message();

        // the max val is 2^12 - 1= 4095
        //int adc_input = adc_read();
        //printf("%d\n", adc_input);
        ////pot_mod = (adc_input >> 1) * pot_divider;
        //pot_mod = adc_input * pot_divider;
    }
}
