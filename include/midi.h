#include <stdint.h>
#include <stdbool.h>

#ifndef MIDI_H
#define MIDI_H

#define MIDI_LED_PIN 15

uint8_t midi_keys[128];
uint8_t midi_previous_keys[128];

void process_midi_commands(uint8_t cmd_fn, uint8_t note, uint8_t velocity);
void on_uart_interrupt(void);

#endif
