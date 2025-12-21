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

const float PI = 3.14159265;
const float filter_mod = PI * 180.0;

const float LFO_MOD = 360.0 / 44100.0;

// XXX TODO SOUND THE GENERAL ALARM: ENV_MAX_TIME_MOD has run out of presision (it doesn't actually matter)
const float ENV_MAX_TIME = 661500.0; // 15s * SAMPLE_RATE
const float ENV_MAX_TIME_MOD = 1.0 / ENV_MAX_TIME;

struct voice voices[VOICE_COUNT] = {0};

float get_amp_mod(float mod) {
    // the real function is 1000^(x-1) but x^4 is used as an aproximation.
    // i should use the real one if i happen to include a math library
    return mod * mod * mod * mod;
}

void initialize_osc(struct osc *osc, waveform selected_waveform) {
    // table_index is not fixed to emulate free running oscillators
    osc->table_increment = 0.0;
    osc->selected_waveform = selected_waveform;
    osc->tune = 0;
    osc->detune = 0;
}

float process_osc(struct osc *osc, int note_increment) {
    osc->tune = note_increment * 100 + osc->detune;
    osc->table_increment = INCREMENT_TABLE[50 + osc->tune];
    osc->table_index += osc->table_increment;
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

void update_osc_waveform(struct osc *osc) {
    osc->selected_waveform++;
    if (osc->selected_waveform == COUNT) {
        osc->selected_waveform = SIN;
    }
}

void update_osc_detune(struct osc *osc, int detune) {
    osc->detune = detune;
}

void initialize_lfo(struct lfo *lfo, float rate, waveform selected_waveform) {
    // table_index is not fixed to emulate free running oscillators
    lfo->table_increment = 0.0;
    lfo->selected_waveform = selected_waveform;
    lfo->rate = rate;
}

float process_lfo(struct lfo *lfo) {
    lfo->table_index += LFO_MOD * lfo->rate;
    if (lfo->table_index > 360.0) {
        lfo->table_index = lfo->table_index - 360.0;
    }
    
    switch (lfo->selected_waveform) {
        case SIN:
            return SIN_TABLE[(int)lfo->table_index];
            break;
       case SAW:
            return SAW_TABLE[(int)lfo->table_index];
            break;
       case SQUARE:
            return SQUARE_TABLE[(int)lfo->table_index];
            break;
        default:
            return 0.0;
    }
}

void update_lfo_waveform(struct lfo *lfo) {
    lfo->selected_waveform++;
    if (lfo->selected_waveform == COUNT) {
        lfo->selected_waveform = SIN;
    }
}

void update_lfo_rate(struct lfo *lfo, float rate) {
    lfo->rate = rate;
}

void initialize_filter(struct filter *f, float cutoff, float resonance, int note) {
    // 0 < cutoff, resonance < 1
    f->cutoff = cutoff;
    f->resonance = resonance;

    f->keytrack = KEYTRACK_TABLE[note];
    f->keytrack_mod = 0.0;

    f->g = compute_filter_g(f->cutoff, f->keytrack, f->keytrack_mod);//2.0 * sinf(PI * f->cutoff / 2.0);
    f->r = 0.5 / f->resonance;
    f->low = f->band = f->high = 0.0;

    f->mode = LOW;
}

float compute_filter_g(float cutoff, float keytrack, float keytrack_mod) {
    float total_cutoff = cutoff + keytrack_mod * (keytrack - cutoff);
    return 2.0 * SIN_TABLE[(int) (filter_mod * total_cutoff)];
}

void update_filter_cutoff(struct filter *f, float cutoff) {
    f->cutoff = cutoff;
    f->g = compute_filter_g(cutoff, f->keytrack, f->keytrack_mod);
}

void update_filter_resonance(struct filter *f, float resonance) {
    f->resonance = resonance;
    f->r = 0.5 / resonance;
}

float process_filter(struct filter *f, float input) {
    f->high = f->r * input - f->low - f->r * f->band;
    f->band += f->g * f->high;
    f->low += f->g * f->band;
    f->notch = f->high + f->low;

    switch (f->mode) {
        case LOW:
            return f->low;
        case BAND:
            return f->band;
        case HIGH:
            return f->high;
        case NOTCH:
            return f->notch;
    }
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

void initialize_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod) {
    e->time = 0;
    e->mod = 0.0;
    e->state = ATTACK;

    // times are used for state calculations
    e->s_mod = s_mod;

    // i am running out of precision on a and d
    set_env_attack_mod(e, a_time_mod);
    set_env_decay_mod(e, d_time_mod);
    set_env_release_mod(e, r_time_mod);
}

// i know that i might do some horrible premature optimization, but it should be quicker.
// you can multiply with the inverse of the max length instead of dividing. might be better.
void set_env_attack_mod(struct env *e, float a_time_mod) {
    //e->a_time = a_time_mod * ENV_MAX_TIME;
    //e->a_mod = a_time_mod * ENV_MAX_TIME_MOD;
    e->a_time = a_time_mod * ENV_MAX_TIME;
    e->a_mod = 1.0 / e->a_time;
}

void set_env_decay_mod(struct env *e, float d_time_mod) {
    //e->d_time = d_time_mod * ENV_MAX_TIME;
    //e->d_mod = (1.0 - e->s_mod) * d_time_mod * ENV_MAX_TIME_MOD;
    e->d_time = d_time_mod * ENV_MAX_TIME;
    e->d_mod = (1.0 - e->s_mod) / e->d_time;
}    

void set_env_release_mod(struct env *e, float r_time_mod) {
    //e->r_time = r_time_mod * ENV_MAX_TIME;
    //e->r_mod = e->s_mod * r_time_mod * ENV_MAX_TIME_MOD;
    e->r_time = r_time_mod * ENV_MAX_TIME;
    e->r_mod = e->s_mod / e->r_time;

    //printf("time:%d, mod:%f, time_mod:%f\n", e->r_time, e->r_mod, r_time_mod);
}

void update_env_a(struct env *e, float time_mod) {
    if (e->state == ATTACK) {
        set_env_attack_mod(e, time_mod);
    }
}

void update_env_d(struct env *e, float time_mod) {
    if (e->state == ATTACK && e->state == DECAY) {
        set_env_decay_mod(e, time_mod);
    }
}

void update_env_r(struct env *e, float time_mod) {
        set_env_release_mod(e, time_mod);
}

void update_env_s(struct env *e, float time_mod) {
    if (e->state == ATTACK && e->state == DECAY) {
        e->s_mod = time_mod;
    }
}

void update_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod) {
    // it should check for if it's new or not in here, or at least not in the set_env funcitons
    switch (e->state) {
        case ATTACK:
            set_env_attack_mod(e, a_time_mod);
            e->s_mod = s_mod;
        case DECAY:
            set_env_decay_mod(e, d_time_mod);
            e->s_mod = s_mod;
        case SUSTAIN:
        case RELEASE:
            set_env_release_mod(e, r_time_mod);
    }
}

void initialize_voice(struct voice *v) {
    v->used = false;
    v->note = 0;
    v->age = 0;
    v->new = false;

    // TODO: uncomment
    //initialize_env(&v->amp_env, 0.1, 0.1, 0.1, 1.0);
    initialize_env(&v->amp_env, 0.0001, 0.0001, 0.0001, 1.0);
    //initialize_env(&v->amp_env, ENV_MAX_TIME_MOD, ENV_MAX_TIME_MOD, ENV_MAX_TIME_MOD, 1.0);
    //initialize_env(&v->filter_env, 0.0001, 0.001, 0.0001, 0.0);

    v->sync = false;
    v->ring_mod = false;
    initialize_osc(&v->osc1, SAW);
    initialize_osc(&v->osc2, SAW);
    // the supersaw sound like a lazer because the phases are the same in the beginning, 
    // which makes them sound louder and out of tune.
    // for the not-very-super saw
    //for (int i = 1; i < 7; i++) {
    //    initialize_osc(&v->oscillators[i], SAW);
    //}

    //initialize_filter(&v->lowpass, 0.5, 0.0, LOWPASS);
    initialize_filter(&v->filter, 0.01, 1.0, v->note);
    initialize_lfo(&v->lfo, 2, SIN);
}


void process_env_r(struct env *e, bool amp) {
    e->mod -= e->r_mod;

    if (e->mod < 0.0) {
        e->mod = 0.0;
    }
    
    e->time++;
}

void process_env_ads(struct env *e) {
    if (e->time < e->a_time) {
        e->mod += e->a_mod;
        e->state = ATTACK;
    } else if (e->time < (e->a_time + e->d_time)) {
        // the decay is starting from where the attack is, but it's still going up
        e->mod -= e->d_mod;
        e->state = DECAY;
    } else {
        // sustain should be at correct level after decay
        // do sustain 
        //e->mod = e->s_mod;
        e->state = SUSTAIN;
    }

    if (e->mod > 1.0) {
        e->mod = 1.0;
    }

    e->time++;
}

// what can happen when the envelopes get reset is that the time wrong.
// it could be bad float arrithmetic. it is not super supported.

// remove and make into two functions
float process_voice(struct voice *v) {
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

            process_env_r(&v->amp_env, true);
            //printf("r:%f\n", v->amp_env.mod);
            //process_env_r(&v->filter_env, true);

            if (v->amp_env.mod <= 0.0) {
                //printf("sound off\n");

                initialize_voice(v);
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
    if (v->amp_env.mod != 0.0) {
        float out = 0;
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

        if (v->ring_mod) {
            out = process_osc(&v->osc1, v->note) * process_osc(&v->osc2, v->note + 7);
        } else {
            out = 0 * process_osc(&v->osc1, v->note + 7) + process_osc(&v->osc2, v->note);
        }
        // ring mod: osc1 * osc2

        // bad supersaw. it's really simple. no pitch tracking allpass filters, just a bit of detuning.
        // sounds nothing like the original
        //float out = 0.0;
        //for (int i = 0; i < 7; i++) {
        //    out += process_osc(&v->oscillators[i], v->note * 100 + max_detune[i] * pot_mod);//0.25);
        //}

        //out = process_filter(&v->lowpass, out, v->filter_env.mod);
        out *= get_amp_mod(v->amp_env.mod); //process_lfo(&v->lfo)
        out = process_filter(&v->filter, out);
        //out = process_filter(&v->lowpass, out, 1.0);

        return out;
    }

    return 0.0;
}

// call it with isr (interrupt service routine)
void on_pwm_interrupt() {
    reset_interrupt();

    float master_out = 0.0;

    for (int i = 0; i < VOICE_COUNT; i++) {
        struct voice *v = &voices[i];
        master_out += process_voice(v);
    }
   
    //master_out = gain * pot_mod * (master_out + gain * pot_mod);
    master_out = GAIN * (master_out + GAIN);

    write_pwm(master_out);
}

