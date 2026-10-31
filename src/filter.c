#include "interface.h"
#include "picosyn.h"

#include "filter.h"
#include "tables.h"

#include <math.h>

const fixed FILTER_CUTOFF_SCALE = FIXED_MAX >> 9;
const fixed FILTER_RESONANCE_SCALE = FIXED_MAX >> 8;

void init_filter(struct filter *f, uint8_t note) {
    // 0 < cutoff, resonance < 1

    f->keytrack = KEYTRACK_TABLE[note];
    f->keytrack_mod = 0;

    // technically they don't need to be initialized here 
    // because they'll be set every cycle.
    //f->f = compute_filter_g(f->cutoff, f->keytrack, f->keytrack_mod);//2.0 * sinf(PI * f->cutoff / 2.0);
    f->low = f->band = f->high = 0;

    f->mode = LOW;
}

void update_filter_cutoff(struct filter *f, uint8_t cutoff) {
    f->cutoff = cutoff * FILTER_CUTOFF_SCALE;
}

void update_filter_resonance(struct filter *f, uint8_t resonance) {
    f->resonance = resonance * FILTER_RESONANCE_SCALE;
}

void update_filter_mode(struct filter *f) {
    // notch mode will not be used
    f->mode++;
    if (f->mode == NOTCH) {
        f->mode = LOW;
    }
}

fixed process_filter(struct filter *f, fixed input, fixed mod) {
    // mod is total modulation from ex. env and lfo
    /*
    ways to have filter cutoff:

    const fixed a = FIXED_MAX / (2 * SAMPLE_RATE);
    factor = a * freq in Hz

    say that factor is factor and roll with it (most probable)

    */ 

    // env mod amt * env mod should be ADDED and not multiplied to this later
    fixed total_cutoff_factor = f->cutoff + mod + mul_fixed(f->keytrack_mod, (f->keytrack - f->cutoff));

    // filtering coefficient
    fixed c = 2 * SIN_TABLE[float_to_fixed(0.01) >> WAVETABLE_SCALE];
    fixed q = float_to_fixed(0.000001);
    //fixed q = f->resonance;
    if (q < 1) {
        q = 1;
    }

    f->high = mul_fixed(q, (input - f->band)) - f->low;
    f->band += mul_fixed(c, f->high);
    f->low += mul_fixed(c, f->band);

    switch (f->mode) {
        case LOW:
            return f->low;
        case BAND:
            return f->band;
        case HIGH:
            return f->high;
        case NOTCH:
            return f->high + f->low;
    }
}

