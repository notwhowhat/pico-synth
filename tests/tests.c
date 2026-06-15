#include "mocks.h"

#include "midi.h"
#include "voice.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

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
        data[i] = process_voice(v);
    }

    // super ugly way to end note, but it does the job
    midi_previous_keys[note] = midi_keys[note];
    midi_keys[note] = 0;
    
    for (int i = 0; i < sampling_duration - note_duration; i++) {
        data[i + note_duration] = process_voice(v);
    }
}

void test_env(struct voice *v) {
    // sustain must be set before release and decay.
    set_env_sustain_mod(&v->amp_env, 0.5);
    set_env_attack_mod(&v->amp_env, 64);
    set_env_decay_mod(&v->amp_env, 64);
    set_env_release_mod(&v->amp_env, 64);

    uint32_t note_duration = 22050;
    uint32_t sampling_duration = 44100;
    fixed data[sampling_duration];

    run_note(v, 60, data, note_duration, sampling_duration);
    write_data(data, sampling_duration);
    plot_data();
}

void test_filter(struct voice *v) {
    // sustain must be set before release and decay.
    set_env_sustain_mod(&v->amp_env, 127);
    set_env_attack_mod(&v->amp_env, 0);
    set_env_decay_mod(&v->amp_env, 0);
    set_env_release_mod(&v->amp_env, 0);

    uint32_t note_duration = 1000;
    uint32_t sampling_duration = 1010;
    fixed data[sampling_duration];

    run_note(v, 60, data, note_duration, sampling_duration);
    write_data(data, sampling_duration);
    plot_data();
}

int main(void) {
    initialize_wavetable(sin_table,      sin_wave);
    initialize_wavetable(triangle_table, triangle_wave);
    initialize_wavetable(sawtooth_table, sawtooth_wave);
    initialize_wavetable(square_table,   square_wave);
    initialize_increment_table();

    initialize_env_mod_tables();

    struct voice v;
    initialize_voice(&v);

    //test_env(&v);
    test_filter(&v);
    
    return 0;

}
