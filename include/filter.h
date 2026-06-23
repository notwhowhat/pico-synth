#include "picosyn.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef FILTER_H
#define FILTER_H

typedef enum {
    LOW, BAND, HIGH, NOTCH, FILTER_COUNT
} filter_mode;

struct filter {
    float cutoff;
    float resonance;

    float keytrack;
    float keytrack_mod;

    float g;
    float r;

    float low;
    float band;
    float high;
    float notch;

    filter_mode mode;
};

void init_filter(struct filter *f, float cutoff, float resonance, int note);
float process_filter(struct filter *f, float input);
float compute_filter_g(float cutoff, float keytrack, float keytrack_mod);
void update_filter_cutoff(struct filter *f, float cutoff);
void update_filter_resonance(struct filter *f, float resonance);

#endif

