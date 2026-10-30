#include "picosyn.h"

#ifndef CONTROLS_H
#define CONTROLS_H

#define BUTTON_COUNT 25
#define POT_COUNT 25

// all updates of voice values should be based on this
/* done update functions:
    VOICE_MODE
    x OSC_A_WAVEFORM
    x OSC_A_COARSE_DETUNE
    x OSC_A_FINE_DETUNE
    x OSC_B_WAVEFORM
    x OSC_B_COARSE_DETUNE
    x OSC_B_FINE_DETUNE
    x? OSC_MIX
    x? OSC_FM
    x OSC_SYNC
    x OSC_RING_MOD
    x AMP_ATTACK
    x AMP_DECAY
    x AMP_SUSTAIN
    x AMP_RELEASE
    x AMP_ENV_MODE
    x MOD_ATTACK
    x MOD_DECAY
    x MOD_SUSTAIN
    x MOD_RELEASE
    x MOD_ENV_MODE
    x FILTER_ATTACK
    x FILTER_DECAY
    x FILTER_SUSTAIN
    x FILTER_RELEASE
    x FILTER_ENV_MODE
    FILTER_MOD
    FILTER_MODE
    FILTER_CUTOFF
    FILTER_RESONANCE
    FILTER_KEYTRACK
    x LFO_A_WAVEFORM
    x LFO_A_RATE
    x LFO_B_WAVEFORM
    x LFO_B_RATE
    x PORTAMENTO_FACTOR
    x CONTROL_COUNT
*/
enum {
    VOICE_MODE,
    OSC_A_WAVEFORM,
    OSC_A_COARSE_DETUNE,
    OSC_A_FINE_DETUNE,
    OSC_B_WAVEFORM,
    OSC_B_COARSE_DETUNE,
    OSC_B_FINE_DETUNE,
    OSC_MIX,
    OSC_FM,
    OSC_SYNC,
    OSC_RING_MOD,
    AMP_ATTACK,
    AMP_DECAY,
    AMP_SUSTAIN,
    AMP_RELEASE,
    AMP_ENV_MODE,
    MOD_ATTACK,
    MOD_DECAY,
    MOD_SUSTAIN,
    MOD_RELEASE,
    MOD_ENV_MODE,
    FILTER_ATTACK,
    FILTER_DECAY,
    FILTER_SUSTAIN,
    FILTER_RELEASE,
    FILTER_ENV_MODE,
    FILTER_MOD,
    FILTER_MODE,
    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_KEYTRACK,
    LFO_A_WAVEFORM,
    LFO_A_RATE,
    LFO_B_WAVEFORM,
    LFO_B_RATE,
    PORTAMENTO_FACTOR,
    CONTROL_COUNT,
};
uint8_t global_controls[CONTROL_COUNT];
bool selected_osc;
bool selected_lfo;
uint8_t selected_env;

// the previous read values
int pots[POT_COUNT];

// the index of controls that is changed
uint16_t prev_values;
int changed_pots[POT_COUNT];
int changed_buttons[BUTTON_COUNT];

int changed_pot_count;
int changed_button_count;

uint8_t bank;
uint8_t preset;

bool load_preset_flag;
bool save_preset_flag;

void init_controls(void);
void init_io_expander(uint8_t hardware_addr, bool isinput);

void process_controls(void);
void get_changed_buttons(void);
void get_changed_pots(void);
void process_changed_buttons(void);
void process_changed_pots(void);

void increment_bank(void);
void increment_preset(void);
void decrement_preset(void);
void save_preset(void);
void load_preset(void);

/*
buttons that toggle features should be updated and checked immediately.
buttons that change which layer to be used will only get updated when a pot updates.
pots will always get updated.
*/
/*
for every button and pot there needs to be an old value stored somewhere.
otherwise. the value of the control should only be changed if they are changed,
even when changing presets.

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