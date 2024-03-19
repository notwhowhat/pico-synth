#ifndef MIDI_H
#define MIDI_H

struct _midi_in {
    byte cmd[3];
    int counter;

    // for regestering inputs
    byte keys[128];
    byte prev_keys[128];

};

typedef struct _midi_in midi_in;
midi_in *midi_in_init();

static byte midi_in_get_raw();

void midi_in_note_on(midi_in *pmidi, byte pitch, byte velocity);
void midi_in_note_off(midi_in *pmidi, byte pitch, byte velocity);

void midi_in_get_cmd(midi_in *pmidi);

void midi_in_parse_cmd(midi_in *pmidi);
#endif




