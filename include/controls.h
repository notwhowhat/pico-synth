#include "voice.h"

#ifndef CONTROLS_H
#define CONTROLS_H

/*
there are a few things needed for the controls to work:
inputs from the pins

*/

typedef enum {
    AMP_A,
    AMP_D,
    AMP_R,
    AMP_S,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    INPUT_COUNT
} inputs;

float controls[INPUT_COUNT];
float prev_controls[INPUT_COUNT];

void process_controls(void);
void update_control(struct voice *v, inputs control, float value);

#endif