#include "controls.h"
#include "picosyn.h"
/*
const int INPUT_COUNT = POT_COUNT + BUTTON_COUNT;

void process_controls(void) {
    changed_pots_counter = 0;
    changed_buttons_counter = 0;
    memset(changed_buttons, 0, sizeof(changed_buttons));
    memset(changed_buttons_number, 0, sizeof(changed_buttons_number));
    memset(changed_pots, 0, sizeof(changed_pots));
    memset(changed_pots_number, 0, sizeof(changed_pots_number));

    check_inputs();

    for (int v = 0; v < VOICE_COUNT; v++) {
        for (int i = 0; i < changed_buttons_counter; i++) {
            process_button_inputs(&voices[v], changed_buttons_number[i], changed_buttons[i]);
        }
        for (int i = 0; i < changed_pots_counter; i++) {
            process_pots_inputs(&voices[v], changed_pots_number[i], changed_pots[i]);
        }
    }
}

void check_inputs(void) {
    // check for new values, change them, and update the voices
    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool input = read_gpio(button_pins[i]);
        if (input != button_inputs[i]) {
            // keep old value for comparing
            changed_buttons[i] = input;
            changed_buttons_number[i] = i;
            changed_buttons_counter++;
        }
    }

    for (int i = 0; i < POT_COUNT; i++) {
        // implement 4th mux pin when added
        write_gpio(MUX_PIN_1, i);
        write_gpio(MUX_PIN_2, i>>1);
        write_gpio(MUX_PIN_3, i>>2);

        int input = adc_read();

        // because of ground noise, this will vary every cycle
        if (input != pot_inputs[i]) {
            // keep old value for comparing
            changed_pots[i] = input;
            changed_pots_number[i] = i;
            changed_pots_counter++;
        }
    }
}

void process_button_inputs(struct voice *v, int changed_button, int value) {
    // will only change when toggled to true. this only works because of booleans.
    if (value) {
        switch (changed_button) {
            case OSC_SELECTED:
                selected_osc = ! selected_osc;
                break;

            case OSC_WAVEFORM:
                update_osc_waveform(selected_osc ? &v->osc1 : &v->osc2);
                break;

            case OSC_SUPER:
                // TODO: implement!
                // v->super = ! v->super;
                break;

            case OSC_SYNC:
                v->sync = ! v->sync;
                break;

            case OSC_RING_MOD:
                v->ring_mod = ! v->ring_mod;
                break;

            case ENV_SELECTED:
                // add third env!
                selected_env = ! selected_env;
                break;

            case FIL_MODE:
                v->filter.mode++;
                if (v->filter.mode == COUNT) {
                    v->filter.mode = LOW;
                }
                break;
            
            case LFO_WAVEFORM:
                update_lfo_waveform(&v->lfo);
                break;

            case LFO_DEST:
                // to be implemented
                break;
        }

        button_inputs[changed_button] = value;
    }
} 


void process_pots_inputs(struct voice *v, int changed_pot, int value) {
    switch (changed_pot) {
        case OSC_TUNE:
            // TODO: adjust value to range
            update_osc_detune(selected_osc ? &v->osc1 : &v->osc2, value);
            break;
        case OSC_BLEND:
            // make exponential maybe
            v->osc2.gain = value * POT_MOD;
            v->osc1.gain = 1.0 - v->osc2.gain;
            break;
        case ENV_ATTACK:
            update_env_a(selected_env ? &v->amp_env : &v->filter_env, value * POT_MOD);
            break;
        case ENV_DECAY:
            update_env_d(selected_env ? &v->amp_env : &v->filter_env, value * POT_MOD);
            break;
        case ENV_SUSTAIN:
            update_env_s(selected_env ? &v->amp_env : &v->filter_env, value * POT_MOD);
            break;
        case ENV_RELEASE:
            update_env_r(selected_env ? &v->amp_env : &v->filter_env, value * POT_MOD);
            break;
        case FIL_CUTOFF:
            update_filter_cutoff(&v->filter, value * POT_MOD);
            break;
        case FIL_RESONANCE:
            update_filter_resonance(&v->filter, value * POT_MOD);
            break;
        case FIL_ENV_AMOUNT:
            v->filter_env_mod = value * POT_MOD;
            break;
        case LFO_RATE:
            // make exponential
            update_lfo_rate(&v->lfo, 205.78 * POT_MOD - 20.58);
            break;
        case LFO_AMOUNT:
            // to be implemented
            break;

    }
}
*/


/*
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
            //update_filter_cutoff(&v->lowpass, value);
            break;
        case FILTER_RESONANCE:
            //update_filter_resonance(&v->lowpass, value);
            break;
    }
}
*/


