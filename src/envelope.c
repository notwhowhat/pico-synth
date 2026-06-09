#include "interface.h"

#include "envelope.h"
#include "picosyn.h"
#include "controls.h"

#include <math.h>

// XXX TODO SOUND THE GENERAL ALARM: ENV_MAX_TIME_MOD has run out of presision (it doesn't actually matter)
const float ENV_MAX_TIME = 661500.0; // 15s * SAMPLE_RATE
const float ENV_MAX_TIME_MOD = 1.0 / ENV_MAX_TIME;

const float ENV_TARGET_CLAMP = 0.0001;
const fixed ENV_FACTOR = (fixed) 0.007 * FIXED_MAX;

// somewhere between virus and nord
const float ENV_MAX_K = 0.03; 
const float ENV_MIN_K = 1e-6; 

/*
linear envelopes don't sound that great. 
a better way to do it is to use a linear acumulator:

    level += (target - env) * k

when k is a coefficient loosely based on time.
because the level results in an asymptote at the target, it
has to be clamped before to make enevelopes run well

k ~ 1 - e ^ -1 / (r * t)

when r is sample rate and t is time.

*/

void initialize_env(struct env *e, env_mode mode, float level) {
    e->mode = mode;
    e->level = level;
    e->state = ATTACK;
}

// i know that i might do some horrible premature optimization, but it should be quicker.
// you can multiply with the inverse of the max length instead of dividing. might be better.

// depth += (target - value) * k;

float get_env_level(struct env *e) {
    return e->level;
}

void set_env_sustain_mod(struct env *e, float s_mod) {
    e->s_mod = s_mod;
}

void set_env_attack_mod(struct env *e, float a_time_mod) {
    e->a_mod = 1.0 / (a_time_mod * ENV_MAX_TIME);
}

void set_env_decay_mod(struct env *e, float d_time_mod) {
    e->d_mod = (1.0 - e->s_mod) / (d_time_mod * ENV_MAX_TIME);
}    

void set_env_release_mod(struct env *e, float r_time_mod) {
    e->r_mod = e->s_mod / (r_time_mod * ENV_MAX_TIME);
}

void update_env_a(struct env *e, float time_mod) {
    if (e->state == ATTACK) {
        set_env_attack_mod(e, time_mod);
    }
}

void update_env_d(struct env *e, float time_mod) {
    if (e->state == ATTACK && e->state == DECAY) {
        set_env_decay_mod(e, time_mod);
    }
}

void update_env_r(struct env *e, float time_mod) {
        set_env_release_mod(e, time_mod);
}

void update_env_s(struct env *e, float time_mod) {
    if (e->state == ATTACK && e->state == DECAY) {
        e->s_mod = time_mod;
    }
}

void process_env_r(struct env *e) {
    if (e->mode == EXP) {
        e->level -= e->level * 0.0007;
    } else {
        e-> level -= e->r_mod;
    }

    if (e->level < ENV_TARGET_CLAMP) {
        e->level = 0.0;
    }
}

void process_env_ads(struct env *e) {
    // TODO: find out why in the world i did not just make mod change until it gets the right value 
    // could it be because i was having problems with mod jumping and becoming negative?
    /*
    this is a hopefully more logical implementation

    if attack: increase mod. if mod > 1: decay.
    if decay: increase mod. if mod > sustain. sustain.
    if sustain: sustain

    implementing legato will be much easier
    
    */
    switch (e->state) {
        case ATTACK:
            if (e->mode == EXP) {
                e->level += (1.0 - e->level) * 0.0007;
            } else {
                e->level += e->a_mod;
            }

            if (e->level > 1.0 - ENV_TARGET_CLAMP) {
                e->level = 1.0;
                e->state = DECAY;
            }
            break;
        case DECAY:
            if (e->mode == EXP) {
                e->level += (e->s_mod - e->level) * 0.0007;
            } else {
                e->level -= e->d_mod;
            }

            if (e->level < e->s_mod + ENV_TARGET_CLAMP) {
                e->level = e->s_mod;
                e->state = SUSTAIN;
            }
            break;
        case SUSTAIN:
            // shouldn't actually be needed
            e->level = e->s_mod;
            break;
    }
}
