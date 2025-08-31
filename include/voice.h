#include "picosyn.h"

//#include "pico/stdlib.h"   // stdlib 
#include <stdint.h>
#include <stdbool.h>

#ifndef VOICE_H
#define VOICE_H

typedef enum {
    SIN,
    SAW,
    SQUARE,
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
    filter_type mode;
    //bool type;

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

struct voice voices[VOICE_COUNT];


void initialize_osc(struct osc *osc, waveform selected_waveform);
float process_osc(struct osc *osc, int note_increment);

void initialize_filter(struct filter *f, float cutoff, float resonance, filter_type mode);
float process_lowpass(struct filter *f, float input);

void initialize_env(struct env *e, int a_time, int d_time, int r_time, float s_mod);
void start_env_r(struct env *e);
void process_env_r(struct env *e, bool amp);
void process_env_ads(struct env *e);
void update_env(struct env *e, int a_time, int d_time, int r_time, float s_mod);

void initialize_voice(struct voice *v);
float process_voice(struct voice *v);

void on_pwm_interrupt(void);

#endif
