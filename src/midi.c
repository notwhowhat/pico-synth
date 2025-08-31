#include "interface.h"
#include "midi.h"
#include "voice.h"
#include "picosyn.h"

//#include "hardware/uart.h"

#include <stdio.h>

uint8_t midi_keys[128] = {0};
uint8_t midi_previous_keys[128] = {0};

void process_midi_commands(uint8_t cmd_fn, uint8_t note, uint8_t velocity) {
    // XXX: different devices output midi note off's in different ways. ableton and my midi keyboard send a ntoe off 
    // command, whilst the mininova just sends a note on with a velocity of zero. both work, but the outputs are set up
    // for the first alternative for performence reasons.
    // TODO: voice stealing doesn't really work. it frees the voice to be stolen, but it doesn't give it a note
    switch(cmd_fn) {
        case 128:
            //gpio_put(MIDI_LED_PIN, 0);
            write_gpio(MIDI_LED_PIN, 0);
            //printf("off @ %d\n", note);
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = 0;
            break;
        case 144:
            //gpio_put(MIDI_LED_PIN, 1);
            write_gpio(MIDI_LED_PIN, 1);
            //printf("on @ %d\n", note);

            // this should work, but it's risky. check if it's a correct value
            midi_previous_keys[note] = midi_keys[note];
            midi_keys[note] = velocity;
            
            struct voice *selected_voice = voices; // &voices[0], because array's are mad

            for (int i = 0; i < VOICE_COUNT; i++) {
                if (!voices[i].used) {
                    selected_voice = &voices[i];
                    break;
                } else {
                    if (voices[i].age < selected_voice->age) {
                        selected_voice = &voices[i];
                    }
                    //if (voices[i].note < selected_voice->note) {
                    //    selected_voice = &voices[i];
                    //}
                }
                voices[i].age++;
            }
            
            selected_voice->note = note;
            selected_voice->used = true;
            selected_voice->new = true;
            break;
        case 176: // cc
            if (note == 1) {
                // modwheel
                //printf("%d", velocity);
            }
            break;
        case 224: // pitch bend
            break;
    }
}

void on_uart_interrupt(void) {
    static int midi_counter = 0;
    static uint8_t midi_cmd[] = {0, 0, 0};
    static uint8_t cmd_fn = 0;
    static uint8_t cmd_channel = 0;

    // it's actually waiting here quite a bit
    //while (!uart_is_readable(uart1)) {
    //}
    //if (uart_is_readable(uart1)) {
    if (check_uart()) {
        uint8_t midi_input = read_uart();
        //printf("%d\n", midi_input);

        midi_cmd[midi_counter] = midi_input;
        midi_counter++;

        // it doesn't send the status byte if it was the same as the last
        // that's the problem! I FOUUUUNNND IT!
        // the status byte always starts with a zero, so it's always bigger than 127!!

        if (midi_counter == 2 && midi_cmd[0] < 128) {
            // this is technicaly unsafe, but come on.
            if (cmd_fn != 0) { // uninitialized
                //printf("short\n");
                process_midi_commands(cmd_fn, midi_cmd[0], midi_cmd[1]);
            }
            
            // process midi with last status byte
            midi_cmd[0] = 0;
            midi_cmd[1] = 0;
            midi_cmd[2] = 0;
            midi_counter = 0;

        } else if (midi_counter == 3) {
            //printf("new\n");
            cmd_fn = midi_cmd[0] & 240;
            cmd_channel = midi_cmd[0] & 15;
            process_midi_commands(cmd_fn, midi_cmd[1], midi_cmd[2]);

            midi_cmd[0] = 0;
            midi_cmd[1] = 0;
            midi_cmd[2] = 0;
            midi_counter = 0;
        }
    }
}