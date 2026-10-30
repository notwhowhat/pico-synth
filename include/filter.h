#include "picosyn.h"

#ifndef FILTER_H
#define FILTER_H

typedef enum {
    LOW, BAND, HIGH, NOTCH, FILTER_COUNT
} filter_mode;

// the filter will have to use q8.24 instead of 1.31.
struct filter {
    float cutoff;
    float resonance;

    float keytrack;
    float keytrack_mod;

    fixed f;
    fixed q;

    fixed low;
    fixed band;
    fixed high;

    filter_mode mode;
};

void init_filter(struct filter *f, float cutoff, float resonance, int note);
fixed process_filter(struct filter *f, fixed input);
float compute_filter_g(float cutoff, float keytrack, float keytrack_mod);
void update_filter_cutoff(struct filter *f, float cutoff);
void update_filter_resonance(struct filter *f, float resonance);

#endif

