#include "interface.h"

#include "controls.h"
#include "picosyn.h"
#include <math.h>

// all controls can be stored with 8 bits, and scaled up later. 
// this is already done for certain controls (the env mod), and is 
// more than enough. i think...

// these sizes can be changed but not increased 

#define IO_EXP_INT_PIN 23
#define MUX_A_PIN 17
#define MUX_B_PIN 19
#define MUX_C_PIN 20

const uint8_t MUX_COUNT = 3;

const float POT_MODIFIER = 1.0 / 4096.0;
const int POT_SCALE = 16; // to scale down from 12-bit to 8-bit
const int CONTROL_NUM = 4;
const int NOISE_THRESHOLD = 10;

// address is 7-bit
const uint8_t IO_EXPANDER_I2C_ADDR = 0x20;
const uint8_t EEPROM_I2C_ADDR = 0x50;

// 64 bytes per preset/page
const uint8_t PRESET_COUNT = 64; // per bank
const uint8_t BANK_COUNT = 8;
const uint16_t PRESET_SIZE = 0x40; // 64 bytes
const uint16_t BANK_SIZE = 0x1000; // 4096 bytes

uint16_t current_preset_addr = 0x00;

const uint8_t EEPROM_SEND_SIZE = CONTROL_COUNT + 2;

void init_controls(void) {
    bank = preset = load_preset_flag = save_preset_flag = 0;
    init_io_expander(0, true);
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

    uint8_t addr = IO_EXPANDER_I2C_ADDR + hardware_addr;

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
        // TODO: add for leds!
    }
}

void process_controls(void) {
    get_changed_buttons();
    process_changed_buttons();

    get_changed_pots();
    process_changed_pots();
    
    // deal with presets here
    if (load_preset_flag) {
        load_preset();
        load_preset_flag = false;
        save_preset_flag = false;
    }
    if (save_preset_flag) {
        save_preset();
        save_preset_flag = false;
    }
    
}

void get_changed_buttons(void) {
    // TODO: do for both expanders maybe :)
    if (read_gpio(IO_EXP_INT_PIN)) {
        changed_button_count = 0;
        //// get which registers have updated (i hope it's usually one)
        //uint8_t intf_reg = 0x0E;
        //uint8_t changed_pins[2];
        //write_i2c(addr, &intf_reg, 1, true);
        //read_i2c(addr, changed_pins, 2, false);
        
        // it is possible to read which pins that has changed instead of new state,
        // but INTCAP has to be cleared anyways, so this is actually faster.
        uint8_t reg = 0x10; // INTCAP
        uint8_t values[2];
        write_i2c(IO_EXPANDER_I2C_ADDR, &reg, 1, true);
        read_i2c(IO_EXPANDER_I2C_ADDR, values, 2, false);

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
                global_controls[OSC_RING_MOD] = !global_controls[OSC_RING_MOD];
                break;
            case 4: // sync
                global_controls[OSC_SYNC] = !global_controls[OSC_SYNC];
                break;
            case 5: // amp env mode
                global_controls[AMP_ENV_MODE] = !global_controls[AMP_ENV_MODE];
                break;
            case 6: // mod env mode
                global_controls[MOD_ENV_MODE] = !global_controls[MOD_ENV_MODE];
                break;
            case 7: // filter env mode
                global_controls[FILTER_ENV_MODE] = !global_controls[FILTER_ENV_MODE];
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
    changed_pot_count = 0;
    for (int i = 0; i < 8; i++) {
        write_gpio(MUX_A_PIN, i & 1); // 1: (1 << 0)
        write_gpio(MUX_B_PIN, (i & 2) >> 1); // 2 : (1 << 1)
        write_gpio(MUX_C_PIN, (i & 4) >> 2); // 4: (1 << 2)

        for (int j = 0; j < MUX_COUNT; j++) {
            select_adc_input(j);
            uint16_t input = read_adc();
            // the pot number
            uint8_t n = i + j * 8;

            if (abs(pots[n] - input) > NOISE_THRESHOLD) {
                pots[i] = input / POT_SCALE;
                changed_pots[changed_pot_count] = n;
                changed_pot_count++;
            }
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

void save_preset(void) {
    uint8_t data[EEPROM_SEND_SIZE] = {0};

    data[0] = (uint8_t)(current_preset_addr >> 8);
    data[1] = (uint8_t)current_preset_addr;
    memcpy(&data[2], global_controls, CONTROL_COUNT);

    write_i2c(EEPROM_I2C_ADDR, data, EEPROM_SEND_SIZE, false);
}

void load_preset(void) {
    uint8_t read_addr[2];
    read_addr[0] = (uint8_t)(current_preset_addr >> 8);
    read_addr[1] = (uint8_t)current_preset_addr;

    write_i2c(EEPROM_I2C_ADDR, read_addr, 2, true);
    read_i2c(EEPROM_I2C_ADDR, global_controls, CONTROL_COUNT, false);
}

void change_bank(void) {
    bank++;
    if (bank > BANK_COUNT) {
        bank = 0;
    }
    current_preset_addr = bank * BANK_SIZE + preset * PRESET_SIZE;
}

void increment_preset(void) {
    preset++;
    if (preset > PRESET_COUNT) {
        preset = 0;
    }
    current_preset_addr = bank * BANK_SIZE + preset * PRESET_SIZE;
}
void decrement_preset(void) {
    preset--;
    if (preset < 0) {
        preset = 0;
    }
    current_preset_addr = bank * BANK_SIZE + preset * PRESET_SIZE;
}

