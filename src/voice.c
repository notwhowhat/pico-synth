#include "interface.h"

#include "voice.h"
#include "tables.h"
#include "midi.h"
#include "picosyn.h"

#include <math.h>

//#include "hardware/pwm.h"  // pwm 

// the next thing to do is to add full modulation. in the beginning, the parameters
// will be fixed, but they'll be added later on. this is done to show the pico's 
// capabilities before impossible features are added.

// the real next thing to do is to figure out if everything implemented is working
// correctly. then, the next feature to add is pitch modulation.

// TODO: logarithmic envelopes. probably won't work.

// TODO: change tuning system to always use and have a tune in cents. do when home, otherwise it'll break.

/*
portamento:
have the portamento modifier be mp
add or subtract mp to the note increment on the following note until 
the right increment is reached. the new note will start with the old one's
note increment. the distance between the notes will determine the time. 

legato:
a new envelope starts where a previous note finishes off.
that should just mean that the new note will get the old one's amp_env_mod

a problem with both legato and portamento is that they require monophony to 
be musically interresting. i will have to add several modes then.

*/

/*
keytracking:
filter frequency can be calculated by taking nyquist * cutoff
the distance to move is freq - note freq 
key cutoff = key frequency / nyquist and is found in table
the note in to be inputted to the table should be in cents, so 100 * midi note

final cutoff = start cutoff + mod * (key cutoff - start cutoff)

ring mod:
out = osc1 * osc2

sync:
when the leader.table_index < leader.table_increment, the 
follower.table_index is set to 0.
*/


const float INV_SAMPLE_RATE = 1.0 / 44100.0;

const float PORTAMENTO_MOD = 0.00009070294;


struct voice voices[VOICE_COUNT] = {0};

void initialize_parameters(struct parameters *p) {
    p->mode = POLY;
    p->portamento_factor = 0.0;
    //p->mode = MONO;
    //p->portamento_factor = 1.0;
}

float get_amp_mod(float mod) {
    // the real function is 1000^(x-1) but x^4 is used as an aproximation.
    // i should use the real one if i happen to include a math library
    return mod * mod * mod * mod;
}

/*
void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode) {
    f->cutoff = cutoff;
    f->resonance = resonance;
    f->mode = mode;

    f->a = f->b = f->c = f->b = 0.0;
}

float process_lowpass(struct filter *f, float input) {
    f->a += ((input - (f->b * f->resonance)) - f->a) * f->cutoff;
    f->b += (f->a - f->b) * f->cutoff;
    f->c += (f->b - f->c) * f->cutoff;
    f->d += (f->c - f->d) * f->cutoff;
    
    return f->d;
}

float process_filter(struct filter *f, float input, float mod) {
    // to acieve a highpass filter, remove the lowpass from the input.
    // highpass = input - lowpass


    // it becomes infinite

    if (f->mode != LOWPASS) {
        f->cutoff = (1.0 - pot_mod);// * mod;
        return input - process_lowpass(f, input);
    } else {
        //f->cutoff = pot_mod * mod;
        return process_lowpass(f, input);
    }
    
}

void update_filter_cutoff(struct filter *f, float cutoff) {
    f->cutoff = cutoff;
}

void update_filter_resonance(struct filter *f, float resonance) {
    f->resonance = resonance;
}
*/


// the minimum time: one sample

void initialize_portamento(struct voice *v, int last_note) {
    if (last_note != -1) {
        // a negative increment and 0.001 of space prevents it from becoming zero
        v->portamento_increment =  (last_note - v->note) * (1.001 - 0.999 * global_paramaters.portamento_factor );//INV_SAMPLE_RATE * (last_note - v->note) * 2000;
        v->portamento = 100 * (last_note - v->note);
    } else {
        v->portamento_increment = v->portamento = 0;
    }
}

void initialize_voice(struct voice *v) {
    v->used = false;
    v->note = -1;
    v->age = 0;
    v->new = false;
    v->portamento = 0.0;

    initialize_env(&v->amp_env, LIN, 0.0);
    // TODO: remove. shouldn't be needed if updated regularly in loop
    set_env_sustain_mod(&v->amp_env, 0.5);
    set_env_attack_mod(&v->amp_env, 0.01);
    set_env_decay_mod(&v->amp_env, 0.01);
    set_env_release_mod(&v->amp_env, 0.01);

    //initialize_env(&v->filter_env, 0.0);

    v->sync = false;
    v->ring_mod = false;
    initialize_osc(&v->osc1, SQUARE);
    initialize_osc(&v->osc2, SQUARE);

    initialize_filter(&v->filter, 0.005, 1.0, v->note);
    initialize_lfo(&v->lfo, 2, SIN);
}

void start_voice_mono_legato(struct voice *v, int note) {
    int last_note = v->note;
    v->note = note;
    v->used = true;
    v->age = 0;


    if (global_paramaters.mode == LEGATO) {
        initialize_env(&v->amp_env, LIN, get_env_level(&v->amp_env));
        initialize_env(&v->filter_env, LIN, get_env_level(&v->filter_env));
    } else {
        initialize_env(&v->amp_env, EXP, 0.0);
        initialize_env(&v->filter_env, EXP, 0.0);
    }

    // max 11025 cycles per semitone as portamento speed
    // portamento speed = 1 / number of cycles per semitone
    //v->portamento_increment = global_paramaters.portamento_factor * PORTAMENTO_MOD * (lv->osc1.tune - v->osc1.tune);
    initialize_portamento(v, last_note);
}

void start_voice_poly(struct voice *v, struct voice *lv, int note) {
    v->note = note;
    v->used = true;
    v->age = 0;

    initialize_env(&v->amp_env, LIN, 0.0);
    initialize_env(&v->filter_env, LIN,  0.0);
    printf("n: %d, l: %d\n", v->note, lv->note);
    initialize_portamento(v, lv->note);
}

void reset_voice(struct voice *v) {
    // the note's age should not be reset. otherwise, determening the distance to slide
    // through portamento is impossible
    v->used = false;
    v->sync = false;
    v->ring_mod = false;

    // envelopes are not reset here to make a future legato toggle possible
    initialize_osc(&v->osc1, SIN);
    initialize_osc(&v->osc2, SIN);

    initialize_filter(&v->filter, 0.01, 1.0, v->note);
    initialize_lfo(&v->lfo, 2, SIN);
}

// remove and make into two functions
fixed process_voice(struct voice *v) {
    // mod is getting -1'd in the 2nd cycle of release
    if (v->used) {

        //printf("mod: %f\n", v->amp_env.mod);
        
        if (midi_keys[v->note] == 0) {
            if (midi_previous_keys[v->note] != 0) {
                // do this with all envelopes

                v->amp_env.state = RELEASE;
                //v->filter_env.state = RELEASE;

                midi_previous_keys[v->note] = 0;
            }

            process_env_r(&v->amp_env);
            //printf("r:%f\n", v->amp_env.mod);
            //process_env_r(&v->filter_env);

            if (v->amp_env.level <= 0) {
                //printf("sound off\n");

                //initialize_voice(v);
                reset_voice(v);
            }
        } else {
            // do this with all enveloeps
            process_env_ads(&v->amp_env);
            //printf("ads:%f\n", v->amp_env.mod);
            //process_env_ads(&v->filter_env);
        }

        //v->amp_env.time++;
    }
    
    // this is where the notes get stuck playing
    //if (midi_keys[voices[i].note] != 0) {
    if (get_env_level(&v->amp_env) != 0) {
        fixed out = 0;
        //printf("mod: %f\n", v->amp_env.mod);

        //v->table_index += INCREMENTS[50 + 100 * v->note];
        //if (v->table_index > 360.0) {
        //    v->table_index = v->table_index - 360.0;
        //}

        //// the gain must be set to something sensible, otherwise the it get's too loud, so the int's
        ////master_out += voices[i].mod * oscillator(voices[i].selected_waveform, voices[i].table_index);
        //return v->amp_env.mod * oscillator(v->selected_waveform, v->table_index);

        //printf("state: %d, a: %f, d: %f, r: %f, s: %f\n", v->amp_env.state,  v->amp_env.a_mod,  v->amp_env.d_mod,  v->amp_env.r_mod,  v->amp_env.s_mod);

        // osc1 is leader for sync.
        if (v->sync) {
            if (v->osc1.table_index < v->osc1.table_increment) {
                v->osc2.table_index = 0.0;
            } 
        }

        if (v->portamento_increment < 0 && v->portamento >= v->portamento_increment ||
            v->portamento_increment > 0 && v->portamento <= v->portamento_increment) {
            v->portamento = 0.0;
        } else {
            v->portamento -= v->portamento_increment;
        }
        
        if (v->ring_mod) {
            out = process_osc(&v->osc1, v->note, v->portamento) * 
                  process_osc(&v->osc2, v->note + 7, v->portamento);
        } else {
            out = process_osc(&v->osc1, v->note, v->portamento);
        }
        // ring mod: osc1 * osc2

        // TODO: uncomment so env works
        //out *= fixed_to_float(v->amp_env.level);

        out = process_filter(&v->filter, out);

        return out;
    }

    return 0;
}

// call it with isr (interrupt service routine)
void on_pwm_interrupt() {
    reset_interrupt();

    float master_out = 0.0;

    for (int i = 0; i < VOICE_COUNT; i++) {
        struct voice *v = &voices[i];
        master_out += process_voice(v);
    }

    if (master_out > 1.0) {
        master_out = 1.0;
    } else if (master_out < -1.0) {
        master_out = -1.0;
    }
   
    //master_out = gain * pot_mod * (master_out + gain * pot_mod);
    // shifts it to the middle
    //master_out = GAIN * (master_out + VOICE_COUNT);
    master_out = GAIN * (master_out + GAIN);

    write_pwm(master_out);
}

