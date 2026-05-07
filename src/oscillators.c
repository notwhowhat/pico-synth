#include "oscillators.h"
#include "tables.h"

#include <math.h>

const float LFO_MOD = 360.0 / SAMPLE_RATE;

void initialize_wavetable(fixed *wavetable, float (*f)(float)) {
    for (int i = 1; i < WAVETABLE_LENGTH; i++) {
        wavetable[i] = (fixed) FIXED_MAX * (*f)(i);
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



void initialize_osc(struct osc *osc, waveform selected_waveform) {
    // table_index is not fixed to emulate free running oscillators
    osc->table_increment = 0.0;
    osc->selected_waveform = selected_waveform;
    osc->tune_cents = 0;
    osc->detune_cents = 0;
    osc->gain = 1.0;
}

float process_osc(struct osc *osc, int note_increment, float portamento) {
    // the portamento approaches zero
    osc->tune_cents = note_increment * 100 + osc->detune_cents + portamento;
    osc->table_increment = INCREMENT_TABLE[50 + osc->tune_cents];
    osc->table_index += osc->table_increment;
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

void update_osc_waveform(struct osc *osc) {
    osc->selected_waveform++;
    if (osc->selected_waveform == COUNT) {
        osc->selected_waveform = SIN;
    }
}

void update_osc_detune(struct osc *osc, int detune) {
    osc->detune_cents = detune;
}

void initialize_lfo(struct lfo *lfo, float rate, waveform selected_waveform) {
    // table_index is not fixed to emulate free running oscillators
    lfo->table_increment = 0.0;
    lfo->selected_waveform = selected_waveform;
    lfo->rate = rate;
}

float process_lfo(struct lfo *lfo) {
    lfo->table_index += LFO_MOD * lfo->rate;
    if (lfo->table_index > 360.0) {
        lfo->table_index = lfo->table_index - 360.0;
    }
    
    switch (lfo->selected_waveform) {
        case SIN:
            return SIN_TABLE[(int)lfo->table_index];
            break;
       case SAW:
            return SAW_TABLE[(int)lfo->table_index];
            break;
       case SQUARE:
            return SQUARE_TABLE[(int)lfo->table_index];
            break;
        default:
            return 0.0;
    }
}

void update_lfo_waveform(struct lfo *lfo) {
    lfo->selected_waveform++;
    if (lfo->selected_waveform == COUNT) {
        lfo->selected_waveform = SIN;
    }
}

void update_lfo_rate(struct lfo *lfo, float rate) {
    lfo->rate = rate;
}


