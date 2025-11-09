#include "controls.h"
#include "picosyn.h"

void process_controls(void) {
    // get the new controls
    // then push them to all voices
    float changed_controls[INPUT_COUNT] = {0};
    int changed_controls_number[INPUT_COUNT] = {0};
    int changed_controls_counter = 0;

    for (int i = 0; i < INPUT_COUNT; i++) {
        if (controls[i] != prev_controls[i]) {
            changed_controls_number[changed_controls_counter] = i;
            changed_controls[changed_controls_counter] = controls[i];
            changed_controls_counter++;
        }
    }

    for (int i = 0; i < changed_controls_counter; i++) {
        for (int j = 0; j < VOICE_COUNT; j++) {
            update_control(&voices[j], changed_controls_number[i], changed_controls[i]);
        }
    }
}

void update_control(struct voice *v, inputs control, float value) {
    // i know this is ugly. a solution with function pointers looks better,
    // but this is readable and maintainable
    switch (control) {
        case AMP_A:
            update_env_a(&v->amp_env, value);
            break;
        case AMP_D:
            update_env_d(&v->amp_env, value);
            break;
        case AMP_R:
            update_env_r(&v->amp_env, value);
            break;
        case AMP_S:
            update_env_s(&v->amp_env, value);
            break;
        case FILTER_CUTOFF:
            update_filter_cutoff(&v->lowpass, value);
            break;
        case FILTER_RESONANCE:
            update_filter_resonance(&v->lowpass, value);
            break;
    }
}


