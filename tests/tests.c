#include "mocks.h"

#include "midi.h"
#include "voice.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void write_data(fixed arr[], uint32_t len) {
    FILE *f = fopen("plot.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < len; i++) {
            fprintf(f, "%d,", i);
        }
        fprintf(f, "\n");
        for (int i = 0; i < len; i++) {
            fprintf(f, "%d,", arr[i]);
        }
    }
    fclose(f);
}

void plot_data(void) {
    system("/bin/python3 /mnt/c/Users/jakob/coding-related/micontroller-projects/npicosyn/scripts/plot.py");
}

void run_note(struct voice *v, uint8_t note, fixed data[], uint32_t note_duration, uint32_t sampling_duration) {
    midi_previous_keys[note] = midi_keys[note];
    midi_keys[note] = 127;
    start_voice_mono_legato(v, note);
    for (int i = 0; i < note_duration; i++) {
        data[i] = (fixed) FIXED_MAX * process_voice(v);
    }

    // super ugly way to end note, but it does the job
    midi_previous_keys[note] = midi_keys[note];
    midi_keys[note] = 0;
    
    for (int i = 0; i < sampling_duration - note_duration; i++) {
        data[i + note_duration] = (fixed) FIXED_MAX * process_voice(v);
    }
}

int main(void) {
    fixed arr[5] = {1, -2, 55, -37, 21};

    struct voice v;
    initialize_voice(&v);

    uint32_t duration = 44100;
    uint32_t note_duration = 22050;
    uint32_t sampling_duration = 44100;
    fixed data[sampling_duration];
    run_note(&v, 60, data, note_duration, sampling_duration);

    write_data(data, sampling_duration);
    plot_data();

    return 0;

}
