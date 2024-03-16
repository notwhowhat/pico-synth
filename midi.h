#ifndef MIDI_H
#define MIDI_H

typedef struct _midi_in midi_in;
midi_in *midi_in_init();

byte midi_in_get_raw();

void midi_in_note_on(midi_in *pmidi, byte pitch, byte velocity);
void midi_in_note_off(midi_in *pmidi, byte pitch, byte velocity);

void midi_in_get_cmd(midi_in *pmidi);

void midi_in_parse_cmd(midi_in *pmidi);
#endif




