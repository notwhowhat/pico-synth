#include "interface.h"
#include "midi.h"
#include "voice.h"
#include "picosyn.h"

//#include "hardware/uart.h"

#include <stdio.h>

#define PPQ 24 // pulses per quarter note

uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

static uint8_t midi_cmd[3] = {0};
static uint8_t midi_counter = 0;
static uint8_t selected_channel = 0;

static uint8_t midi_cmd_len = 3;
static bool channel_is_known = false; 

void note_off(int note) {
    write_gpio(MIDI_LED_PIN, 0);
    //printf("off @ %d\n", note);
    midi_previous_keys[note] = midi_keys[note];
    midi_keys[note] = 0;
}

void note_on(int note, int velocity) {
    //gpio_put(MIDI_LED_PIN, 1);
    write_gpio(MIDI_LED_PIN, 1);
    //printf("on @ %d\n", note);

    // this should work, but it's risky. check if it's a correct value
    midi_previous_keys[note] = midi_keys[note];
    midi_keys[note] = velocity;
            
    if (global_paramaters.mode == POLY) {
        struct voice *selected_voice = voices; // &voices[0], because array's are mad
        struct voice *last_voice = voices;
        bool theft = false;
        for (int i = 0; i < VOICE_COUNT; i++) {
            if (!voices[i].used) {
                selected_voice = &voices[i];
            } else if (voices[i].age > selected_voice->age) {
                // this voice is being stolen. it's info should be passed for legato
                // portamento should be done elsewhere
                // this should not be else if. 
                selected_voice = &voices[i];
                theft = true;
            }

            if (voices[i].age > 0 && voices[i].age < last_voice->age) {
                last_voice = &voices[i];
            }

            voices[i].age++;
        }
        if (theft) {
            reset_voice(selected_voice);
        }
        // somehow selected voice and last voice become the same.
        start_voice_poly(selected_voice, last_voice, note);
    } else {
        start_voice_mono_legato(voices, note);
    }
}

void process_midi_commands(uint8_t s1, uint8_t s2, uint8_t d1, uint8_t d2) {
    // XXX: different devices output midi note off's in different ways. ableton and my midi keyboard send a ntoe off 
    // command, whilst the mininova just sends a note on with a velocity of zero. both work, but the outputs are set up
    // for the first alternative for performence reasons.
    // TODO: voice stealing doesn't really work. it frees the voice to be stolen, but it doesn't give it a note
    //printf("cmd: %d, note: %d, velocity: %d\n", cmd_fn, note, velocity);
    // status will be top 4 bits in byte. 0xS0: S will be status
    switch(s1) {
        case 0x80: // note off
            //gpio_put(MIDI_LED_PIN, 0);
            if (s2 == selected_channel) {
                note_off(d1); // note 
            }
            break;
        case 0x90: // note on
            if (s2 == selected_channel) {
                if (d2 > 0) { // velocity
                    note_on(d1, d2);
                } else {
                    note_off(d1);
                }
            }
            break;
        case 0xC0: // preset change
            break;
        case 0xE0: // pitch bend
            break;
        case 0xF0: // gloabl parameters
            midi_cmd_len = 1;
            switch (s2) {
                case 0x00: // sysex 
                    break;
                case 0x02: // SPP
                    midi_cmd_len = 3;
                    break;
                case 0x07: // end sysex
                    break;
                case 0x08: // clock
                    break;
                case 0x0A: // start
                    break;
                case 0x0B: // continue
                    break;
                case 0x0C: // stop
                    break;
                case 0x0F: // total reset
                    break;
            }
            break;
    }
}

void reset_midi_cmd(len) {
    switch (len) {
        case 1:
            midi_cmd[0] = midi_cmd[1];
            midi_cmd[1] = midi_cmd[2];
            midi_cmd[2] = 0;
            midi_counter = 2;
            break;
        case 2:
            midi_cmd[0] = midi_cmd[2];
            midi_cmd[1] = 0;
            midi_cmd[2] = 0;
            midi_counter = 1;
            break;
        case 3:
            midi_cmd[0] = 0;
            midi_cmd[1] = 0;
            midi_cmd[2] = 0;
            midi_counter = 0;
            break;
    }
}

void on_uart_interrupt(void) {
    static uint8_t s1 = 0;
    static uint8_t s2 = 0;

    if (check_uart()) {
        uint8_t midi_input = read_uart();
        //printf("%d\n", midi_input);

        midi_cmd[midi_counter] = midi_input;
        midi_counter++;

        if (midi_counter == 3) {
            if (midi_cmd < 128) {
                if (channel_is_known) {
                    // running state
                    // status byte is ommited if note on/off on same ch
                    midi_cmd_len = 2;
                    process_midi_commands(s1, s2, midi_cmd[1], midi_cmd[2]);
                }
            } else {
                s1 = midi_cmd[0] & 0xF0;
                s2 = midi_cmd[0] & 0x0F;
                channel_is_known = true;

                midi_cmd_len = 3;
                process_midi_commands(s1, s2, midi_cmd[1], midi_cmd[2]);
            }           

            reset_midi_cmd(midi_cmd_len);
        }
    }
}
