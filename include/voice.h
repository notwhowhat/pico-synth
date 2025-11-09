#include "picosyn.h"

//#include "pico/stdlib.h"   // stdlib 
#include <stdint.h>
#include <stdbool.h>

#ifndef VOICE_H
#define VOICE_H

// TODO: add triangle wave
typedef enum {
    SIN,
    SAW,
    SQUARE,
    COUNT
} waveform;

typedef enum {
    ATTACK, DECAY, SUSTAIN, RELEASE,
} env_state;

typedef enum {
    LOWPASS, HIGHPASS
} filter_type;

struct env {
    int time;
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
};

struct filter {
    float cutoff;
    float resonance;
    filter_type mode;
    //bool type;

    // buffers for orders of filter
    float a;
    float b;
    float c;
    float d;
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

extern struct voice voices[VOICE_COUNT];


void initialize_osc(struct osc *osc, waveform selected_waveform);
float process_osc(struct osc *osc, int note_increment);

void update_osc_waveform(struct osc *osc);
void update_osc_tune(struct osc *osc, int tune);

void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode);
float process_lowpass(struct filter *f, float input);
void update_filter_cutoff(struct filter *f, float cutoff);
void update_filter_resonance(struct filter *f, float resonance);

void initialize_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod);
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

void initialize_voice(struct voice *v);
float process_voice(struct voice *v);

void on_pwm_interrupt(void);

#endif
