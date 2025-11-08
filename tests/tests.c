#include "mocks.h"

#include "midi.h"
#include "voice.h"

#include <stdio.h>
#include <assert.h>

int main(void) {
    //struct env tenv;
    //initialize_env(&tenv, 1, 1, 1, 0.5);

    //printf("a: %f, d: %f, r: %f, s: %f\n", tenv.a_mod, tenv.d_mod, tenv.r_mod, tenv.s_mod);

    struct voice v;
    initialize_voice(&v);
    v.used = true;
    v.new = true;

    midi_keys[v.note] = 1;
    midi_previous_keys[v.note] = 1;
    
    for (int i = 0; i < 20; i++) {
        process_voice(&v);
        printf("mod: %f\n", v.amp_env.mod);
    }
    printf("note off\n");

    midi_keys[v.note] = 0;
    midi_previous_keys[v.note] = 1;

    for (int i = 0; i < 30; i++) {
        process_voice(&v);
        printf("mod: %f\n", v.amp_env.mod);
    }
    
    printf("success\n");

    return 0;

}
