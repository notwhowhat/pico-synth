#include "picosyn.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef OSCILLATORS_H
#define OSCILLATORS_H

#define WAVETABLE_LENGTH 256

typedef enum {
    SIN,
    SAW,
    SQUARE,
    COUNT
} waveform;

struct osc {
    float table_index;
    float table_increment;
    waveform selected_waveform;
    int tune_cents;
    int detune_cents;
    float gain;
};

struct lfo {
    float table_index;
    float table_increment;
    waveform selected_waveform;
    float rate;
};

extern const float LFO_MOD;

fixed sin_table[WAVETABLE_LENGTH];
fixed square_table[WAVETABLE_LENGTH];
fixed triangle_table[WAVETABLE_LENGTH];
fixed sawtooth_table[WAVETABLE_LENGTH];

void initialize_wavetable(fixed *wavetable, float (*f)(float));
float sin_wave(float x);
float square_wave(float x);
float triangle_wave(float x);
float sawtooth_wave(float x);

void initialize_osc(struct osc *osc, waveform selected_waveform);
float process_osc(struct osc *osc, int note_increment, float portamento);
void update_osc_waveform(struct osc *osc);
void update_osc_detune(struct osc *osc, int detune);

void initialize_lfo(struct lfo *lfo, float rate, waveform selected_waveform);
float process_lfo(struct lfo *lfo);
void update_lfo_waveform(struct lfo *lfo);
void update_lfo_rate(struct lfo *lfo, float rate);

#endif