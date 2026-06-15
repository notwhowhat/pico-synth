#include "interface.h"

#include "envelope.h"
#include "picosyn.h"
#include "controls.h"

#include <math.h>

// XXX TODO SOUND THE GENERAL ALARM: ENV_MAX_TIME_MOD has run out of presision (it doesn't actually matter)

const double ENV_TARGET_CLAMP_FLOAT = 0.0001;
const fixed ENV_TARGET_CLAMP = (fixed)(ENV_TARGET_CLAMP_FLOAT * FIXED_MAX);
const fixed ENV_FACTOR = (fixed)(0.0007 * FIXED_MAX);

const double ENV_MIN_TIME = 0.001; // s
const double ENV_MAX_TIME = 30; // s

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

void initialize_env_mod_tables(void) {
    for (int i = 0; i < ENV_MOD_TABLE_LENGTH; i++) {
        // t = min * (max / min) ^ x
        float t = ENV_MIN_TIME * pow(ENV_MAX_TIME / ENV_MIN_TIME, (double)i / (double)ENV_MOD_TABLE_LENGTH);

        // k = 1 / rt
        env_mod_table_lin[i] = (fixed)(FIXED_MAX / (SAMPLE_RATE * t));

        // k = 1 - err ^ (1 / rt)
        env_mod_table_exp[i] = (fixed)(FIXED_MAX * (1.0 - pow(ENV_TARGET_CLAMP_FLOAT, 1.0 / (SAMPLE_RATE * t))));
    }
}

void initialize_env(struct env *e, env_mode mode, fixed level) {
    // TODO: make it possible to use linear mode.
    // right now the time is not set up.
    e->mode = mode;
    e->level = level;
    e->state = ATTACK;
}

// i know that i might do some horrible premature optimization, but it should be quicker.
// you can multiply with the inverse of the max length instead of dividing. might be better.

// depth += (target - value) * k;

float get_env_level(struct env *e) {
    return fixed_to_float(e->level);
}

void set_env_sustain_mod(struct env *e, uint8_t mod) {
    // TODO: scale properly
    e->s_mod = FIXED_MAX;//FIXED_MAX - (FIXED_MAX >> 2);
}

// TODO: CHANGE ALL MOD FUNCTIONS!
void set_env_attack_mod(struct env *e, uint8_t mod) {
    //e->a_mod = 1.0 / (a_time_mod * ENV_MAX_TIME);
    //e->a_mod = ENV_FACTOR;
    e->a_mod = env_mod_table_exp[mod];
}

void set_env_decay_mod(struct env *e, uint8_t mod) {
    //e->d_mod = (1.0 - e->s_mod) / (d_time_mod * ENV_MAX_TIME);
    //e->d_mod = ENV_FACTOR;
    e->d_mod = env_mod_table_exp[mod];
}    

void set_env_release_mod(struct env *e, uint8_t mod) {
    //e->r_mod = e->s_mod / (r_time_mod * ENV_MAX_TIME);
    //e->r_mod = ENV_FACTOR;
    e->r_mod = env_mod_table_exp[mod];
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

void update_env_mode(struct env *e) {
    if (e->mode == LIN) {
        e->mode == EXP;
    } else {
        e->mode == LIN;
    }
}

void process_env_r(struct env *e) {
    if (e->mode == EXP) {
        e->level -= mul_fixed(e->level, e->r_mod);
    } else {
        e-> level -= e->r_mod;
    }

    if (e->level < ENV_TARGET_CLAMP) {
        e->level = 0;
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
                e->level += mul_fixed((FIXED_MAX - e->level), e->a_mod);
            } else {
                e->level += e->a_mod;
            }

            if (e->level > FIXED_MAX - ENV_TARGET_CLAMP) {
                e->level = FIXED_MAX;
                e->state = DECAY;
            }
            break;
        case DECAY:
            if (e->mode == EXP) {
                e->level += mul_fixed((e->s_mod - e->level), e->d_mod);
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
