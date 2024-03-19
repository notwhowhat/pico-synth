#include <Arduino.h>
#include "midi.h"

//#define CHANNEL 0

//typedef struct _midi_in midi_in;

midi_in *midi_in_init() {
    midi_in *p= (midi_in*)malloc(sizeof(midi_in));
    if (p != NULL) {

        p->cmd[0] = 0;
        p->cmd[1] = 0;
        p->cmd[2] = 0;

        p->counter = 0;

        for (int i = 0; i < 127; i++) {
            p->keys[i] = 0;
            p->prev_keys[i] = 0;
        }
    } else {
        Serial.println("WRONG");
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

static byte midi_in_get_raw() {
    while (Serial1.available() < 1) {
    }
    int buffer = Serial1.read();
    if (buffer < 0) {
        return 0;
    }
    return (byte)buffer;
}

void midi_in_note_on(midi_in *pmidi, byte pitch, byte velocity) {
#ifdef PRINT
    Serial.println("on");
#endif
    pmidi->prev_keys[pitch] = pmidi->keys[pitch];
    pmidi->keys[pitch] = velocity;
}

void midi_in_note_off(midi_in *pmidi, byte pitch, byte velocity) {
#ifdef PRINT
    Serial.println("off");
#endif
    pmidi->prev_keys[pitch] = pmidi->keys[pitch];
    pmidi->keys[pitch] = 0;
}

void midi_in_get_cmd(midi_in* pmidi) {
    int input = midi_in_get_raw();

    if (input != 0) {

        // forces it to have data
        switch (pmidi->counter) {
            case 0:
                // status byte
                pmidi->cmd[0] = input;
                pmidi->counter++;
                break;
            case 1:
                pmidi->cmd[1] = input;
                pmidi->counter++;
                break;
            case 2:
                pmidi->cmd[2] = input;
                pmidi->counter = 0;
                break;
        }

    }

}

void midi_in_parse_cmd(midi_in* pmidi) {
    //midi_in midi = *pmidi;


    if (pmidi->cmd[2] != 0) {
    //if (1) {

        //Serial.println("READING");
        //Serial.println("PARSING");

        // interpret the status byte
        byte status = pmidi->cmd[0] & 240;
        byte channel = pmidi->cmd[0] & 15;

        // only one available channel
        //if (channel == CHANNEL) {
        if (1) {

            // TODO: expand the number of cmds
            switch (status) {
                case 128:
                    midi_in_note_off(pmidi, pmidi->cmd[1], pmidi->cmd[2]);
                    //Serial.println("off");
                    //Serial.println(midi.cmd[1]);
                    //Serial.println(midi.cmd[2]);
                    break;
                case 144:
                    midi_in_note_on(pmidi, pmidi->cmd[1], pmidi->cmd[2]);
                    //Serial.println("on");
                    //Serial.println(pmidi->cmd[1]);
                    //Serial.println(pmidi->cmd[2]);
                    break;
            }

            // resets the message
            pmidi->cmd[0] = 0;
            pmidi->cmd[1] = 0;
            pmidi->cmd[2] = 0;

        }
    }
}


