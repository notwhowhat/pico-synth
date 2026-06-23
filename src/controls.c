#include "interface.h"

#include "controls.h"
#include "picosyn.h"
#include <math.h>

// all controls can be stored with 8 bits, and scaled up later. 
// this is already done for certain controls (the env mod), and is 
// more than enough. i think...

// these sizes can be changed but not increased 

#define IO_EXP_INT_PIN 23

const size_t CONTROL_COUNT = sizeof(struct controls);
const uint8_t BANK_COUNT = 8;
const uint8_t BANK_SIZE = 64;

const float POT_MODIFIER = 1.0 / 4096.0;
const int POT_SCALE = 16; // to scale down from 12-bit to 8-bit
const int CONTROL_NUM = 4;
const int NOISE_THRESHOLD = 10;

// address is 7-bit
static uint8_t addr = 0x20;

void init_controls(void) {
    for (int i; i < CONTROL_NUM -1; i++) {
        controls[i] = -1.0;
    }
}

void init_io_expander(uint8_t hardware_addr, bool isinput) {
    // TODO: init gpio pin
    /*
    interrupts on the chip will be used to save read cycles.
    the pins INTA and INTB will be or'ed together by IOCON.MIRROR = 1
    either pin will remain high until values are read.
    at startup IOCON.BANK = 0, which


    these should be set up for 0x and 1x to work for A and B
    the rest are set to the right value in the beginning

    0x04 GPINTEN: ioc toggle (0xFF, on)
    0x0A IOCON: config (0x6A (01101010), see p.21)
    0x06 GPPU: gpio pullup (0xFF, on)
    */

    uint8_t addr = 0x20 + hardware_addr;

    if (isinput) {
        // IOCON: config (0x6A (01101010), see p.21)
        // after this, one write will change the corresponding register for A and B
        uint8_t config_data[2] = {0x0A, 0x6A};
        write_i2c(addr, config_data, 2, false);

        uint8_t data[3];

        // GPINTEN: interrupt on change toggle (0xFF, on)
        data[0] = 0x04;
        data[1] = data[2] = 0xFF;
        write_i2c(addr, data, 3, false);
    
        // GPPU: gpio pullup (0xFF, on)
        data[0] = 0x0C;
        data[1] = data[2] = 0xFF;
        write_i2c(addr, data, 3, false);

    } else {
        // TODO: add!
    }
}

void process_controls(void) {
    get_changed_buttons();
    process_changed_buttons();

    get_changed_pots();
    process_changed_pots();
    
    // deal with presets here
}

void get_changed_buttons(void) {
    if (read_gpio(IO_EXP_INT_PIN)) {
        //// get which registers have updated (i hope it's usually one)
        //uint8_t intf_reg = 0x0E;
        //uint8_t changed_pins[2];
        //write_i2c(addr, &intf_reg, 1, true);
        //read_i2c(addr, changed_pins, 2, false);
        
        // it is possible to read which pins that has changed instead of new state,
        // but INTCAP has to be cleared anyways, so this is actually faster.
        uint8_t reg = 0x10; // INTCAP
        uint8_t values[2];
        write_i2c(addr, &reg, 1, true);
        read_i2c(addr, values, 2, false);

        uint16_t new_values = ((uint16_t)values[0] << 8) + (uint16_t)values[1];
        uint16_t changes = new_values ^ prev_values;
        prev_values = new_values;
        
        for (int i = 0; i < 16; i++) {
            if (new_values & 0x01) {
                // the smallest bit is on and the value has changed! 
                // the actual button values do not matter if the chip
                // pics up the correct changes
                changed_buttons[changed_button_count] = i;
                changed_button_count++;
                
                new_values >> 1;
            }
        }
    }
    //changed_button_count = 0;


    //for (int i = 0; i < BUTTON_COUNT; i++) {
    //    // should be taken in with i2c chip. right now, the values will just be the same
    //    int input = read_gpio(pin);
    //    
    //    if (input != buttons[i]) {
    //        buttons[i] = input;
    //        changed_buttons[changed_button_count] = i;
    //        changed_button_count++;
    //    }
    //}
}

// not done
void process_changed_buttons(void) {
    for (int i = 0; i < changed_button_count; i++) {

        // cycle stuff in control struct
        // TODO: trigger leds
        switch (changed_buttons[i]) {
            // these cases are just button numbers that can and 
            // probably will be changed later
            case 0: // selected osc
                selected_osc = !selected_osc;
                break;
            case 1: // selected env
                selected_env++;
                if (selected_env > 2) { // env count - 1
                    selected_env = 0;
                }
                break;
            case 2: // selected lfo
                selected_lfo = !selected_lfo;
                break;
            case 3: // ring mod
                global_controls.osc_ring_mod = !global_controls.osc_ring_mod;
                break;
            case 4: // sync
                global_controls.osc_sync = !global_controls.osc_sync;
                break;
            case 5: // amp env mode
                global_controls.amp_env_mode = !global_controls.amp_env_mode;
                break;
            case 6: // mod env mode
                global_controls.mod_env_mode = !global_controls.mod_env_mode;
                break;
            case 7: // filter env mode
                global_controls.filter_env_mode = !global_controls.filter_env_mode;
                break;
            case 8: // bank
                change_bank();
                break;
            case 9: // preset up
                increment_preset();
                break;
            case 10: // preset down
                decrement_preset();
                break;
            case 11: // load preset
                if (changed_buttons[i] == true) {
                    load_preset_flag = true;
                }
                break;
            case 12: // save -||-
                if (changed_buttons[i] == true) {
                    save_preset_flag = true;
                }
            default:
                break;
        }
    }
}

void get_changed_pots(void) {
    // does all sorts of muxing and sends updated pots to be processed
    changed_pot_count = 0;
    for (int i = 0; i < POT_COUNT; i++) {
        // the inputs should be read from the muxes but now only one is connected.
        int input = read_adc();

        if (abs(pots[i] - input) > NOISE_THRESHOLD) {
            pots[i] = input / POT_SCALE;
            changed_pots[changed_pot_count] = i;
            changed_pot_count++;
        }
    }
}


void process_changed_pots(void) {
    // no function pointers, because the function paramaters must be of the same type
    // most pots are dependent on states of buttons
    for (int i = 0; i < changed_pot_count; i++) {
        for (int j = 0; j < VOICE_COUNT; j++) {
            switch (changed_pots[i]) { // which pots have been changed?
                case 0: // osc coarse tune
                    if (selected_osc) {
                        update_osc_coarse_detune(&voices[j].osc_a, pots[i]);
                    } else {
                        update_osc_coarse_detune(&voices[j].osc_b, pots[i]);
                    }
                    break;
                case 1: // osc fine tune
                    if (selected_osc) {
                        update_osc_fine_detune(&voices[j].osc_a, pots[i]);
                    } else {
                        update_osc_fine_detune(&voices[j].osc_b, pots[i]);
                    }
                    break;
                case 2: // osc mix
                    // TODO: find some good way to scale to fixed full range
                    voices[j].mix = FIXED_MIN;

                    break;
            }
        }
    }
}


void save_preset(struct parameters *p) {
    save_preset_flag = false;
    uint8_t data[CONTROL_COUNT];
    for (int i = 0; i < CONTROL_COUNT; i++) {
        // cast parameters as union and iterate over values as array.
    }
}

void load_preset(struct parameters *p) {
    load_preset_flag = false;
}

void change_bank(void) {
    bank++;
    if (bank > BANK_COUNT) {
        bank = 0;
    }
}

void increment_preset(void) {
    preset++;
    if (preset > BANK_SIZE) {
        preset = 0;
    }
}
void decrement_preset(void) {
    preset--;
    if (preset < 0) {
        preset = 0;
    }
}




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


