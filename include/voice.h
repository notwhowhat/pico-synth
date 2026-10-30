#include "picosyn.h"

#include "envelope.h"
#include "filter.h"
#include "oscillators.h"

#ifndef VOICE_H
#define VOICE_H

// TODO: add triangle wave

typedef enum {
    MONO, LEGATO, POLY, VOICE_MODE_COUNT,
} voice_mode;
voice_mode mode;

// the voice gets allocated a note, which it reads
struct voice {
    bool used;
    int note; // which note in midi_keys it's connected to
    int last_note;
    int age;
    bool new;

    bool sync;
    bool ring_mod;
    struct osc osc_a;
    struct osc osc_b;
    fixed mix;
    fixed fm; // TODO: implement! ratio of how much to add osc a to osc b

    fixed portamento_increment;
    fixed portamento;

    //struct osc oscillators[7];

    struct env amp_env;
    struct env mod_env;
    struct env filter_env;
    float filter_env_mod;

    struct filter filter;

    struct lfo lfo_a;
    struct lfo lfo_b;
};

struct parameters {
    voice_mode mode;

    waveform osc_a_waveform; 
    waveform osc_b_waveform; 
    float osc_a_tune;
    float osc_b_tune;
    float osc_a_detune;
    float osc_b_detune;

    float osc_mix;
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

void init_parameters(struct parameters *p);
//void init_filter(struct filter *f, float cutoff, float resonance, int note);
//float process_filter(struct filter *f, float input);
//float compute_filter_g(float cutoff, float keytrack, float keytrack_mod);
//void update_filter_cutoff(struct filter *f, float cutoff);
//void update_filter_resonance(struct filter *f, float resonance);
//void init_filter(struct filter *f, float cutoff, float resonance, filter_type mode);
//float process_lowpass(struct filter *f, float input);
//void update_filter_cutoff(struct filter *f, float cutoff);
//void update_filter_resonance(struct filter *f, float resonance);
float get_amp_mod(float mod);

void init_voice(struct voice *v);
//void start_voice(struct voice *v, struct voice *nv, int note);
void start_voice_mono_legato(struct voice *v, int note);
void start_voice_poly(struct voice *v, struct voice *lv, int note);
void reset_voice(struct voice *v);
fixed process_voice(struct voice *v);

void update_osc_ring_mod(struct voice *v);
void update_osc_sync(struct voice *v);
void update_portamento(struct voice *v, fixed mod);

void on_pwm_interrupt(void);

#endif
