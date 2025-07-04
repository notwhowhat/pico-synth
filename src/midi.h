#ifndef MIDI
#define MIDI

void process_midi_commands(uint8_t cmd_fn, uint8_t note, uint8_t velocity);
void process_midi(void);

#endif
