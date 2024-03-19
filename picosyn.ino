#include <Arduino.h>
#include "midi.h"

void setup() {
    pinMode(25, OUTPUT);

    // only for print debuging :D
    Serial.begin(9600);

    Serial1.setRX(1);
    Serial1.begin(31250);

    // wait untill ready, otherwise no printing or input
    while (!Serial) {
    }
    while (!Serial1) {
    }


    midi_in *midi = midi_in_init();
    if (midi != NULL) {
        Serial.println("initialized");
    }

    int toggle = 0;

    while (1) {

        //if (midi->counter > 0) {
            //Serial.println(midi->counter);
        //}
        //Serial.println("INFINITY");
        midi_in_get_cmd(midi);
        midi_in_parse_cmd(midi);

        //if (midi->keys[60] != 0) {
        //    Serial.println(midi->keys[60]);
        //}


        // this doesn't work, cause prev keys and keys don't get reset the right way.

        if (midi->counter == 0) {
            for (int i = 0; i < 127; i++) {
                if (midi->keys[i] != 0 && midi->prev_keys[i] == 0) { // it's on, but for how long? 
                    //Serial.println("NOTE ON");
                    Serial.print("Note on at: midi ");
                    Serial.print(i);
                    Serial.print(" velocity ");
                    Serial.println(midi->keys[i]);
                    midi->prev_keys[i] = midi->keys[i];
                    toggle = 1;
                } else if (midi->keys[i] == 0 && midi->prev_keys[i] != 0) {
                    Serial.print("Note off at: midi ");
                    Serial.println(i);
                    midi->prev_keys[i] = midi->keys[i];
                    toggle = 0;
                }
            }
        }

        if (toggle == 1) {
            digitalWrite(25, HIGH);
        } else if (toggle == 0) {
            digitalWrite(25, LOW);
        }

        //Serial.print("hi");
        //delay(1000);
        //Serial.print("bye");
        //delay(1000);

    }
}

void loop() {
    // keep this empty, run everything in setup 
}

