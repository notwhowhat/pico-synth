#include "interface.h"

#include "voice.h"
#include "tables.h"
#include "midi.h"
#include "picosyn.h"

//#include "hardware/pwm.h"  // pwm 


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

void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode) {
    f->cutoff = cutoff;
    f->resonance = resonance;
    f->mode = mode;

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

    if (f->mode != LOWPASS) {
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
    e->state = ATTACK;

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
        case ATTACK:
            e->a_time = a_time;
            e->a_mod = 1.0 / e->a_time;
        case DECAY:
            e->d_time = d_time;
            e->d_mod = (1.0 - e->s_mod) / e->d_time;
        //case S:
        //    e->s_mod = s_mod;
        case RELEASE:
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
    e->state = RELEASE;
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
        e->state = ATTACK;
    } else if (e->time < (e->a_time + e->d_time)) {
        // do decay
        // the decay is starting from where the attack is, but it's still going up
        e->mod -= e->d_mod;
        e->state = DECAY;
    } else {
        // do sustain 
        e->mod = e->s_mod;
        e->state = SUSTAIN;
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
    //pwm_clear_irq(pwm_gpio_to_slice_num(AUDIO_PIN));    
    reset_interrupt();

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
    master_out = GAIN * (master_out + GAIN);

    //pwm_set_gpio_level(AUDIO_PIN, (uint16_t)master_out);
    write_pwm(master_out);
    //uint32_t d_time = time_us_32() - start;
    //if (d_time > slowest_time) {
    //    slowest_time = d_time;
    //    printf("%d\n", slowest_time);
    //}
}

