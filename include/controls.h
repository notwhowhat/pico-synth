#include "voice.h"

#ifndef CONTROLS_H
#define CONTROLS_H

/*
buttons that toggle features should be updated and checked immediately.
buttons that change which layer to be used will only get updated when a pot updates.
pots will always get updated.
*/

/*
const float POT_MOD = 1.0 / 4095.0;

// all are gotten through mux
static const int MUX_PIN_1 = 18;
static const int MUX_PIN_2 = 19;
static const int MUX_PIN_3 = 20;


enum pot_mappings {
    OSC_TUNE,
    OSC_BLEND,
    ENV_ATTACK,
    ENV_DECAY,
    ENV_SUSTAIN,
    ENV_RELEASE,
    FIL_CUTOFF,
    FIL_RESONANCE,
    FIL_ENV_AMOUNT,
    LFO_RATE,
    LFO_AMOUNT,
    POT_COUNT
};

// add preset and arp buttons
enum button_mappings {
    OSC_SELECTED,
    OSC_WAVEFORM,
    OSC_SUPER,
    OSC_SYNC,
    OSC_RING_MOD,
    ENV_SELECTED,
    FIL_MODE,
    LFO_WAVEFORM,
    LFO_DEST,
    BUTTON_COUNT
};

int button_pins[BUTTON_COUNT] = {
    [OSC_SELECTED] = -1,
    [OSC_WAVEFORM] = -1,
    [OSC_SUPER] = -1,
    [OSC_SYNC] = -1,
    [OSC_RING_MOD] = -1,
    [ENV_SELECTED] = -1,
    [FIL_MODE] = -1,
    [LFO_DEST] = -1,
};

int button_inputs[BUTTON_COUNT];
int pot_inputs[POT_COUNT];

bool selected_osc;
bool selected_env;

int changed_pots[POT_COUNT];
int changed_pots_number[POT_COUNT];
int changed_pots_counter;

int changed_buttons[BUTTON_COUNT];
int changed_buttons_number[POT_COUNT];
int changed_buttons_counter;

*/

/*
typedef enum {
    AMP_A,
    AMP_D,
    AMP_R,
    AMP_S,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    INPUT_COUNT
} inputs;
*/

/*
int pots[POT_COUNT];
int prev_pots[POT_COUNT];

int buttons[BUTTON_COUNT];
int prev_buttons[BUTTON_COUNT];
*/

//float controls[INPUT_COUNT];
//float prev_controls[INPUT_COUNT];

//void process_controls(void);
//void update_control(struct voice *v, inputs control, float value);

#endif