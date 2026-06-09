#include "oscillators.h"
#include "tables.h"

#include <math.h>

const float LFO_MOD = 360.0 / SAMPLE_RATE;

void initialize_wavetable(float *table, float (*f)(float)) {
    for (int i = 0; i < WAVETABLE_LENGTH; i++) {
        table[i] =  (fixed)(f(2.0 * M_PI * i) * FIXED_MAX);
    }
}

// the functions are used to generate wave tables. wave periods are all 1.0
float sin_wave(float x) {
    return sin(2.0 * M_PI * x);
}

float square_wave(float x) {
    return 2.0 * floor(sin(2.0 * M_PI * x)) + 1;
}

float sawtooth_wave(float x) {
    return 2.0 * fmod(x, 1.0) - 1;
}

float triangle_wave(float x) {
    return asin(sin(2.0 * M_PI * x)) * 2.0 / M_PI;
}

void initialize_increment_table() {
    for (int i = 0; i < INCREMENT_TABLE_LENGTH; i++) {
        float f = 440.0 * pow(2.0, (i - 100 * 69.5) / 1200.0); // 69 is midi for 440 Hz
        // fixed max is used to scale: table size * scale = 2^32 (same as max)
        increment_table[i] = (uint32_t) UINT32_MAX * (f / SAMPLE_RATE); 
    }
}

void initialize_osc(struct osc *osc, waveform selected_waveform) {
    osc->table_increment = 0;
    osc->selected_waveform = 0;
    osc->tune_cents = 0;
    osc->detune_cents = 0;
    osc->selected_waveform = selected_waveform;
    osc->gain = 0;
}

fixed process_osc(struct osc *osc, int note, float portamento) {
    // the portamento approaches zero
    osc->tune_cents = note * 100 + osc->detune_cents;// + portamento;
    osc->table_increment = increment_table[50 + osc->tune_cents];

    osc->table_index += osc->table_increment;
    uint32_t index = osc->table_index >> INCREMENT_SCALE;
    
    switch (osc->selected_waveform) {
        case SIN:
            return sin_table[(int)index];
            break;
        case TRI:
            return triangle_table[(int)index];
            break;
        case SAW:
            return sawtooth_table[(int)index];
            break;
        case SQUARE:
            return square_table[(int)index];
            break;
        default:
            return 0;
    }
}

void update_osc_waveform(struct osc *osc) {
    osc->selected_waveform++;
    if (osc->selected_waveform == COUNT) {
        osc->selected_waveform = SIN;
    }
}

void update_osc_detune(struct osc *osc, int detune) {
    osc->detune_cents = detune;
}

void initialize_lfo(struct lfo *lfo, uint32_t rate, waveform selected_waveform) {
    // table_index is not fixed to emulate free running oscillators
    lfo->selected_waveform = selected_waveform;
    lfo->rate = rate;
}

fixed process_lfo(struct lfo *lfo) {
    lfo->table_index += (UINT32_MAX - lfo->rate);
    uint32_t index = lfo->table_index >> INCREMENT_SCALE;
    
    switch (lfo->selected_waveform) {
        case SIN:
            return sin_table[(int)index];
            break;
        case TRI:
            return triangle_table[(int)index];
            break;
        case SAW:
            return sawtooth_table[(int)index];
            break;
        case SQUARE:
            return square_table[(int)index];
            break;
        default:
            return 0;
    }
}

void update_lfo_waveform(struct lfo *lfo) {
    lfo->selected_waveform++;
    if (lfo->selected_waveform == COUNT) {
        lfo->selected_waveform = SIN;
    }
}

void update_lfo_rate(struct lfo *lfo, uint32_t rate) {
    lfo->rate = rate;
}
