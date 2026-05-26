#include "picosyn.h"

//#include "pico/stdlib.h"   // stdlib 
#include <stdint.h>
#include <stdbool.h>

#include "envelope.h"
#include "filter.h"
#include "oscillators.h"

#ifndef VOICE_H
#define VOICE_H

// TODO: add triangle wave

typedef enum {
    MONO, LEGATO, POLY
} voice_mode;


// the voice gets allocated a note, which it reads
struct voice {
    bool used;
    int note; // which note in midi_keys it's connected to
    int age;
    bool new;

    bool sync;
    bool ring_mod;
    struct osc osc1;
    struct osc osc2;

    float portamento_increment;
    float portamento;

    //struct osc oscillators[7];

    struct env amp_env;
    struct env filter_env;
    float filter_env_mod;

    //struct filter lowpass;
    struct filter filter;

    struct lfo lfo;
};

struct parameters {
    voice_mode mode;

    waveform osc_a_waveform; 
    waveform osc_b_waveform; 
    float osc_a_tune;
    float osc_b_tune;
    float osc_a_detune;
    float osc_b_detune;

    float osc_blend;
    float osc_fm;
    bool osc_sync;
    bool osc_ring_mod;

    float amp_attack;
    float amp_decay;
    float amp_sustain;
    float amp_release;
    float amp_mod;

    float mod_attack;
    float mod_decay;
    float mod_sustain;
    float mod_release;

    float filter_attack;
    float filter_decay;
    float filter_sustain;
    float filter_release;
    float filter_mod;

    float filter_mode;
    float filter_cutoff;
    float filter_resonance;
    float filter_keytrack;

    waveform lfo_a_waveform;
    waveform lfo_b_waveform;
    float lfo_a_rate;
    float lfo_b_rate;

    float portamento_factor;

} parameters;

struct parameters global_paramaters;
extern struct voice voices[VOICE_COUNT];

void initialize_parameters(struct parameters *p);
//void initialize_filter(struct filter *f, float cutoff, float resonance, int note);
//float process_filter(struct filter *f, float input);
//float compute_filter_g(float cutoff, float keytrack, float keytrack_mod);
//void update_filter_cutoff(struct filter *f, float cutoff);
//void update_filter_resonance(struct filter *f, float resonance);
//void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode);
//float process_lowpass(struct filter *f, float input);
//void update_filter_cutoff(struct filter *f, float cutoff);
//void update_filter_resonance(struct filter *f, float resonance);
float get_amp_mod(float mod);

void initialize_voice(struct voice *v);
//void start_voice(struct voice *v, struct voice *nv, int note);
void start_voice_mono_legato(struct voice *v, int note);
void start_voice_poly(struct voice *v, struct voice *lv, int note);
void reset_voice(struct voice *v);
fixed process_voice(struct voice *v);

void on_pwm_interrupt(void);

#endif
