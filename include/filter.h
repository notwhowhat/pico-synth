#include "picosyn.h"

#ifndef FILTER_H
#define FILTER_H

typedef enum {
    LOW, BAND, HIGH, NOTCH, FILTER_COUNT
} filter_mode;

struct filter {
    fixed cutoff; // from 0 -> FIXED_MAX / 2
    fixed resonance; // from 0 -> FIXED_MAX

    fixed keytrack;
    fixed keytrack_mod;

    fixed f;
    fixed q;

    fixed low;
    fixed band;
    fixed high;

    filter_mode mode;
};

void init_filter(struct filter *f, uint8_t note);
fixed process_filter(struct filter *f, fixed input, fixed mod);
void update_filter_cutoff(struct filter *f, uint8_t cutoff);
void update_filter_resonance(struct filter *f, uint8_t resonance);
void update_filter_mode(struct filter *f);

#endif

