#include "picosyn.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef OSCILLATORS_H
#define OSCILLATORS_H

// it's actually good if the table length is 256
#define WAVETABLE_LENGTH 256

// 128 midi notes + 1 for detune, and 100 cents/semitone
#define INCREMENT_TABLE_LENGTH 12900 
#define INCREMENT_SCALE 24 // 32 - log2(wavetable length)

typedef enum {
    SIN,
    TRI,
    SAW,
    SQUARE,
    COUNT
} waveform;

struct osc {
    // even though they are fixed, the table values are treated as q8.24
    uint32_t table_index;
    uint32_t table_increment;
    waveform selected_waveform;
    int tune_cents;
    int detune_cents;
    fixed gain;
};

struct lfo {
    uint32_t table_index;
    waveform selected_waveform;
    uint32_t rate;
};


//struct lfo {
//    float table_index;
//    float table_increment;
//    waveform selected_waveform;
//    float rate;
//};

extern const float LFO_MOD;
float sin_table[WAVETABLE_LENGTH];
float square_table[WAVETABLE_LENGTH];
float triangle_table[WAVETABLE_LENGTH];
float sawtooth_table[WAVETABLE_LENGTH];
uint32_t increment_table[INCREMENT_TABLE_LENGTH];

void initialize_wavetable(float *table, float (*f)(float));
void initialize_increment_table(void);

float sin_wave(float x);
float square_wave(float x);
float triangle_wave(float x);
float sawtooth_wave(float x);

void initialize_osc(struct osc *osc, waveform selected_waveform);
fixed process_osc(struct osc *osc, int note, float portamento);
void update_osc_waveform(struct osc *osc);
void update_osc_detune(struct osc *osc, int detune);

void initialize_lfo(struct lfo *lfo, uint32_t rate, waveform selected_waveform);
fixed process_lfo(struct lfo *lfo);
void update_lfo_waveform(struct lfo *lfo);
void update_lfo_rate(struct lfo *lfo, uint32_t rate);

//void initialize_lfo(struct lfo *lfo, float rate, waveform selected_waveform);
//float process_lfo(struct lfo *lfo);
//void update_lfo_waveform(struct lfo *lfo);
//void update_lfo_rate(struct lfo *lfo, float rate);

#endif