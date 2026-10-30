#include "picosyn.h"

#ifndef MIDI_H
#define MIDI_H

#define MIDI_LED_PIN 15

uint8_t midi_keys[128];
uint8_t midi_previous_keys[128];

void process_midi_commands(uint8_t s1, uint8_t s2, uint8_t d1, uint8_t d2);
void on_uart_interrupt(void);

#endif
