#include <Arduino.h>
#include "midi.h"

#define CHANNEL 0

struct _midi_in {
    byte cmd[3];
    int counter;

    // for regestering inputs
    byte keys[128];
    byte prev_keys[128];

};

typedef struct _midi_in midi_in;

midi_in *midi_in_init() {
    midi_in *p= (midi_in*)malloc(sizeof(midi_in));
    if (p != NULL) {
        p->cmd[0] = 0;
        p->cmd[1] = 0;
        p->cmd[2] = 0;

        p->counter = 0;

        for (int i; i < 127; i++) {
            p->keys[i] = 0;
            p->prev_keys[i] = 0;
        }
    }
    return p;
    



    //midi_in midi;

    //midi.counter = 0;
    //midi.cmd[0] = 0;
    //midi.cmd[1] = 0;
    //midi.cmd[2] = 0;

    //for (int i; i < 127; i++) {
    //    midi.keys[i] = 0;
    //    midi.prev_keys[i] = 0;
    //}

    //return &midi;
}

byte midi_in_get_raw() {
    int buffer = Serial1.read();
    if (buffer < 0) {
        return 0;
    }
    return (byte)buffer;
}

void midi_in_note_on(midi_in *pmidi, byte pitch, byte velocity) {
    midi_in midi = *pmidi;
    midi.prev_keys[pitch] = midi.keys[pitch];
    midi.keys[pitch] = velocity;
}

void midi_in_note_off(midi_in *pmidi, byte pitch, byte velocity) {
    midi_in midi = *pmidi;
    midi.prev_keys[pitch] = midi.keys[pitch];
    midi.keys[pitch] = velocity;
}

void midi_in_get_cmd(midi_in* pmidi) {
    midi_in midi = *pmidi;
    int input = midi_in_get_raw();

    if (input != 0) {
        // forces it to have data
        switch (midi.counter) {
            case 0:
                // status byte
                midi.cmd[0] = input;
                midi.counter++;
                break;
            case 1:
                midi.cmd[1] = input;
                midi.counter++;
                break;
            case 2:
                midi.cmd[2] = input;
                midi.counter = 0;
                break;
        }

    }

}

void midi_in_parse_cmd(midi_in* pmidi) {
    midi_in midi = *pmidi;

    if (midi.cmd[2] != 0) {
        // interpret the status byte
        byte status = midi.cmd[0] & 240;
        byte channel = midi.cmd[0] & 15;

        // only one available channel
        if (channel == CHANNEL) {

            // TODO: expand the number of cmds
            switch (status) {
                case 128:
                    midi_in_note_off(pmidi, midi.cmd[1], midi.cmd[2]);
                    Serial.println("off");
                    Serial.println(midi.cmd[1]);
                    Serial.println(midi.cmd[2]);
                    break;
                case 144:
                    midi_in_note_off(pmidi, midi.cmd[1], midi.cmd[2]);
                    Serial.println("on");
                    Serial.println(midi.cmd[1]);
                    Serial.println(midi.cmd[2]);
                    break;
            }
            //midi.cmd = {0, 0, 0};
            midi.cmd[0] = 0;
            midi.cmd[1] = 0;
            midi.cmd[2] = 0;

        }
    }
}


