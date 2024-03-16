#include <Arduino.h>
#include "midi.h"

void setup() {
    Serial.begin(9600);
    Serial1.begin(31250);

    while (1) {
        Serial.print("hi");
        delay(1000);
        Serial.print("bye");
        delay(1000);

    }
}

void loop() {
    // keep this empty, run everything in setup 
}

