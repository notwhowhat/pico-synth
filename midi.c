#include <Arduino.h>

#define CHANNEL 0

typedef struct midi_in {
    byte cmd[3];
    int counter;

    // for regestering inputs
    byte keys[128];
    byte prev_keys[128];

} midi_in;

midi_in* midi_in_init() {
    midi_in midi;

    midi.counter = 0;
    midi.cmd = {0, 0, 0};

    for (int i; i < 127; i++) {
        midi.keys[i] = 0;
        midi.prev_keys[i] = 0;
    }

    return &midi;
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

    if (midi != 0) {
        // forces it to have data
        switch (midi.counter) {
            case 0:
                // status byte
                midi.command[0] = input;
                midi.counter++;
                break;
            case 1:
                midi.command[1] = input;
                midi.counter++;
                break;
            case 2:
                midi.command[2] = input;
                midi.counter = 0;
                break;
        }

    }

}

void midi_in_parse_cmd(midi_in* pmidi) {
    midi_in midi = *pmidi;

    if (midi.command[2] != 0) {
        // interpret the status byte
        byte status = midi.command[0] & 240;
        byte channel = midi.command[0] & 15;

        // only one available channel
        if (channel == CHANNEL) {

            // TODO: expand the number of commands
            switch (status) {
                case 128:
                    midi_in_note_off(pmidi, midi.command[1], midi.command[2]);
                    Serial.println("off");
                    Serial.println(midi.command[1]);
                    Serial.println(midi.command[2]);
                    break;
                case 144:
                    midi_in_note_off(pmidi, midi.command[1], midi.command[2]);
                    Serial.println("on");
                    Serial.println(midi.command[1]);
                    Serial.println(midi.command[2]);
                    break;
            }
            midi.command = {0, 0, 0}
        }
    }
}














int m_in_counter = 0;

typedef struct m_cmd {
    byte a;
    byte b;
    byte c;
} m_cmd;



m_cmd m_in_cmd;

byte m_in() {
    int buffer = Serial1.read();
    if (buffer < 0) {
        return 0;
    }
    return (byte)buffer;
}

byte m_keys[128];
byte m_prev_keys[128];

void m_note_off(byte keys[128], byte prev_keys[128], byte pitch, byte velocity) {
    m_prev_keys[pitch] = m_keys[pitch];
    m_keys[pitch] = velocity;
}
void m_note_on(byte keys[128], byte prev_keys[128], byte pitch, byte velocity) {
    m_prev_keys[pitch] = m_keys[pitch];
    m_keys[pitch] = velocity;
}

void m_get_cmd(m_cmd* cmd, int counter) {
    m_cmd m_in_cmd = *cmd
    int midi = m_in();

    // runs through and sees if there is data.
    if (midi != 0) {
        //Serial.println(midi);
        switch (m_in_counter) {
            case 0:
                // status byte
                m_in_cmd.a = midi;
                m_in_counter++;
                break;
            case 1:
                // second byte
                m_in_cmd.b = midi;
                m_in_counter++;
                break;
            case 2:
                // third byte
                m_in_cmd.c = midi;
                m_in_counter = 0;
        }
    }
}

void m_parse_cmd(m_cmd* cmd) {
    if (m_in_cmd.c != NULL) {
        // parse the whole command
        byte m_status = m_in_cmd.a & 240;
        byte m_channel = m_in_cmd.a & 15;

        if (m_channel == CHANNEL) {
            // only one channel now

            switch (m_status) {
                case 128:
                    m_note_off(m_keys, m_prev_keys, m_in_cmd.b, m_in_cmd.c);
                    Serial.println("off");
                    Serial.println(m_in_cmd.b);
                    Serial.println(m_in_cmd.c);

                    break;
                case 144:
                    m_note_on(m_keys, m_prev_keys, m_in_cmd.b, m_in_cmd.c);
                    Serial.println("on");
                    Serial.println(m_in_cmd.b);
                    Serial.println(m_in_cmd.c);

                    break;
            }
            m_in_cmd = {};
        }
    }
}



