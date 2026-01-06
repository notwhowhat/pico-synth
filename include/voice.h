#include "picosyn.h"

//#include "pico/stdlib.h"   // stdlib 
#include <stdint.h>
#include <stdbool.h>

#ifndef VOICE_H
#define VOICE_H

extern const float DEFAULT_ENV_MOD;

// TODO: add triangle wave

typedef enum {
    MONO, LEGATO, POLY
} voice_mode;

typedef enum {
    SIN,
    SAW,
    SQUARE,
    COUNT
} waveform;

typedef enum {
    ATTACK, DECAY, SUSTAIN, RELEASE,
} env_state;

// TODO: add max count
typedef enum {
    LOW, BAND, HIGH, NOTCH, FILTER_COUNT
} filter_mode;

struct env {
    float mod;
    env_state state;

    float a_time;
    float d_time;
    float r_time;
   
    float s_mod;
    float a_mod;
    float d_mod;
    float r_mod;
};

struct osc {
    float table_index;
    float table_increment;
    waveform selected_waveform;
    int tune;
    int detune;
    float gain;
};

struct lfo {
    float table_index;
    float table_increment;
    waveform selected_waveform;
    float rate;
};

struct filter {
    float cutoff;
    float resonance;

    float keytrack;
    float keytrack_mod;

    float g;
    float r;

    float low;
    float band;
    float high;
    float notch;

    filter_mode mode;
};


//struct filter {
    //float cutoff;
    //float resonance;
    //filter_type mode;
    ////bool type;

    //// buffers for orders of filter
    //float a;
    //float b;
    //float c;
    //float d;
//};

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

void initialize_osc(struct osc *osc, waveform selected_waveform);
float process_osc(struct osc *osc, int note_increment, float portamento);
void update_osc_waveform(struct osc *osc);
void update_osc_detune(struct osc *osc, int detune);

void initialize_lfo(struct lfo *lfo, float rate, waveform selected_waveform);
float process_lfo(struct lfo *lfo);
void update_lfo_waveform(struct lfo *lfo);
void update_lfo_rate(struct lfo *lfo, float rate);

void initialize_filter(struct filter *f, float cutoff, float resonance, int note);
float process_filter(struct filter *f, float input);
float compute_filter_g(float cutoff, float keytrack, float keytrack_mod);
void update_filter_cutoff(struct filter *f, float cutoff);
void update_filter_resonance(struct filter *f, float resonance);
//void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode);
//float process_lowpass(struct filter *f, float input);
//void update_filter_cutoff(struct filter *f, float cutoff);
//void update_filter_resonance(struct filter *f, float resonance);

void initialize_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod, float mod);
void process_env_r(struct env *e, bool amp);
void process_env_ads(struct env *e);

// these set_env_mod functions are technically not needed if it's updated every cycle
void set_env_attack_mod(struct env *e, float a_time_mod);
void set_env_decay_mod(struct env *e, float d_time_mod);
void set_env_release_mod(struct env *e, float r_time_mod);

void update_env_a(struct env *e, float time_mod);
void update_env_d(struct env *e, float time_mod);
void update_env_r(struct env *e, float time_mod);
void update_env_s(struct env *e, float time_mod);
void update_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod); // should not be used.

float get_amp_mod(float mod);

void initialize_voice(struct voice *v);
void start_voice(struct voice *v, struct voice *nv, int note);
void reset_voice(struct voice *v);
float process_voice(struct voice *v);

void on_pwm_interrupt(void);

#endif
