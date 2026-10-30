#include "interface.h"
#include "picosyn.h"

#include "filter.h"
#include "tables.h"

#include <math.h>

void init_filter(struct filter *f, float cutoff, float resonance, int note) {
    // 0 < cutoff, resonance < 1
    f->cutoff = cutoff;
    f->resonance = resonance;

    f->keytrack = KEYTRACK_TABLE[note];
    f->keytrack_mod = 0.0;

    // technically they don't need to be initialized here 
    // because they'll be set every cycle.
    //f->f = compute_filter_g(f->cutoff, f->keytrack, f->keytrack_mod);//2.0 * sinf(PI * f->cutoff / 2.0);
    f->q = 0.5 / f->resonance;
    f->low = f->band = f->high = 0.0;

    f->mode = LOW;
}

float compute_filter_g(float cutoff, float keytrack, float keytrack_mod) {
    float total_cutoff = cutoff + keytrack_mod * (keytrack - cutoff);
    //return 2.0 * SIN_TABLE[(int) (M_PI * 180.0 * total_cutoff)];
    // TODO: look in to this. 
    // it might be a bit wrong because right now, the table isn't based on pi
    // g = 2 sin (pi * cutoff / fs) //[approximately]
    // f0 = (MAX / (2 * fs))
    // f1 = (log2(MAX) - log2(WAVETABLE_LENGTH))
    // g = 2(sin((f0 * cutoff) >> f1)))
    return 2.0 * SIN_TABLE[(int) (180.0 * total_cutoff)];
}

void update_filter_cutoff(struct filter *f, float cutoff) {
    f->cutoff = cutoff;
    f->f = compute_filter_g(cutoff, f->keytrack, f->keytrack_mod);
}

void update_filter_resonance(struct filter *f, float resonance) {
    f->resonance = resonance;
    f->q = 0.5 / resonance;
}

fixed process_filter(struct filter *f, fixed input) {
    // TODO: refactor to use an envelope generator 
    /*
    ways to have filter cutoff:

    const fixed a = FIXED_MAX / (2 * SAMPLE_RATE);
    factor = a * freq in Hz

    say that factor is factor and roll with it (most probable)

    */ 

    // env mod amt * env mod should be ADDED and not multiplied to this later
    fixed total_cutoff_factor = f->cutoff + f->keytrack_mod * (f->keytrack - f->cutoff);

    f->f = 2 * SIN_TABLE[float_to_fixed(0.01) >> WAVETABLE_SCALE];
    f->q = float_to_fixed(0.000001);

    f->high = mul_fixed(f->q, (input - f->band)) - f->low;
    f->band += mul_fixed(f->f, f->high);
    f->low += mul_fixed(f->f, f->band);

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

