#include "picosyn.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef OSCILLATORS_H
#define OSCILLATORS_H

// it's actually good if the table length is 256
#define WAVETABLE_LENGTH 1024

// 128 midi notes + 1 for detune, and 100 cents/semitone
#define INCREMENT_TABLE_LENGTH 12900 
#define INCREMENT_SCALE 22 // 32 - log2(wavetable length)

typedef enum {
    SIN,
    TRI,
    SAW,
    SQUARE,
    WAVEFORM_COUNT
} waveform;

struct osc {
    // even though they are fixed, the table values are treated as q8.24
    uint32_t table_index;
    uint32_t table_increment;
    waveform selected_waveform;
    int tune_cents;
    int detune_cents;
    int detune_semitones;
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
fixed sin_table[WAVETABLE_LENGTH];
fixed square_table[WAVETABLE_LENGTH];
fixed triangle_table[WAVETABLE_LENGTH];
fixed sawtooth_table[WAVETABLE_LENGTH];
uint32_t increment_table[INCREMENT_TABLE_LENGTH];

void update_waveform(waveform w);

void init_wavetable(fixed *table, float (*f)(float));
void init_increment_table(void);

float sin_wave(float x);
float square_wave(float x);
float triangle_wave(float x);
float sawtooth_wave(float x);

void init_osc(struct osc *osc, waveform selected_waveform);
fixed process_osc(struct osc *osc, int note, float portamento);
void update_osc_waveform(struct osc *osc);
void update_osc_coarse_detune(struct osc *osc, uint8_t detune);
void update_osc_fine_detune(struct osc *osc, uint8_t detune);
void update_osc_mix(struct osc *osc_a, struct osc *osc_b, fixed mix);

void init_lfo(struct lfo *lfo, uint32_t rate, waveform selected_waveform);
fixed process_lfo(struct lfo *lfo);
void update_lfo_waveform(struct lfo *lfo);
void update_lfo_rate(struct lfo *lfo, uint32_t rate);

//void init_lfo(struct lfo *lfo, float rate, waveform selected_waveform);
//float process_lfo(struct lfo *lfo);
//void update_lfo_waveform(struct lfo *lfo);
//void update_lfo_rate(struct lfo *lfo, float rate);

#endif