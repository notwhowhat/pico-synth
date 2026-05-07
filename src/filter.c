#include "interface.h"
#include "picosyn.h"

#include "filter.h"
#include "tables.h"

#include <math.h>

void initialize_filter(struct filter *f, float cutoff, float resonance, int note) {
    // 0 < cutoff, resonance < 1
    f->cutoff = cutoff;
    f->resonance = resonance;

    f->keytrack = KEYTRACK_TABLE[note];
    f->keytrack_mod = 0.0;

    f->g = compute_filter_g(f->cutoff, f->keytrack, f->keytrack_mod);//2.0 * sinf(PI * f->cutoff / 2.0);
    f->r = 0.5 / f->resonance;
    f->low = f->band = f->high = 0.0;

    f->mode = LOW;
}

float compute_filter_g(float cutoff, float keytrack, float keytrack_mod) {
    float total_cutoff = cutoff + keytrack_mod * (keytrack - cutoff);
    //return 2.0 * SIN_TABLE[(int) (M_PI * 180.0 * total_cutoff)];
    return 2.0 * SIN_TABLE[(int) (M_PI * 180.0 * total_cutoff)];
}

void update_filter_cutoff(struct filter *f, float cutoff) {
    f->cutoff = cutoff;
    f->g = compute_filter_g(cutoff, f->keytrack, f->keytrack_mod);
}

void update_filter_resonance(struct filter *f, float resonance) {
    f->resonance = resonance;
    f->r = 0.5 / resonance;
}

float process_filter(struct filter *f, float input) {
    f->high = f->r * input - f->low - f->r * f->band;
    f->band += f->g * f->high;
    f->low += f->g * f->band;
    f->notch = f->high + f->low;

    switch (f->mode) {
        case LOW:
            return f->low;
        case BAND:
            return f->band;
        case HIGH:
            return f->high;
        case NOTCH:
            return f->notch;
    }
}

