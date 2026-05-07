#include "interface.h"

#include "envelope.h"
#include "picosyn.h"

#include <math.h>

// XXX TODO SOUND THE GENERAL ALARM: ENV_MAX_TIME_MOD has run out of presision (it doesn't actually matter)
const float ENV_MAX_TIME = 661500.0; // 15s * SAMPLE_RATE
const float ENV_MAX_TIME_MOD = 1.0 / ENV_MAX_TIME;

void initialize_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod, float level) {
    e->level = level;
    e->state = ATTACK;

    // times are used for state calculations
    e->s_mod = s_mod;

    // i am running out of precision on a and d
    set_env_attack_mod(e, a_time_mod);
    set_env_decay_mod(e, d_time_mod);
    set_env_release_mod(e, r_time_mod);
}

// i know that i might do some horrible premature optimization, but it should be quicker.
// you can multiply with the inverse of the max length instead of dividing. might be better.

// TODO: make the envelopes nonlinear
// depth += (target - value) * k;
void set_env_attack_mod(struct env *e, float a_time_mod) {
    //e->a_time = a_time_mod * ENV_MAX_TIME;
    //e->a_mod = a_time_mod * ENV_MAX_TIME_MOD;
    e->a_time = a_time_mod * ENV_MAX_TIME;
    e->a_mod = 1.0 / e->a_time;
}

void set_env_decay_mod(struct env *e, float d_time_mod) {
    //e->d_time = d_time_mod * ENV_MAX_TIME;
    //e->d_mod = (1.0 - e->s_mod) * d_time_mod * ENV_MAX_TIME_MOD;
    e->d_time = d_time_mod * ENV_MAX_TIME;
    e->d_mod = (1.0 - e->s_mod) / e->d_time;
}    

void set_env_release_mod(struct env *e, float r_time_mod) {
    //e->r_time = r_time_mod * ENV_MAX_TIME;
    //e->r_mod = e->s_mod * r_time_mod * ENV_MAX_TIME_MOD;
    e->r_time = r_time_mod * ENV_MAX_TIME;
    e->r_mod = e->s_mod / e->r_time;

    //printf("time:%d, mod:%f, time_mod:%f\n", e->r_time, e->r_mod, r_time_mod);
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

void update_env(struct env *e, float a_time_mod, float d_time_mod, float r_time_mod, float s_mod) {
    // it should check for if it's new or not in here, or at least not in the set_env funcitons
    switch (e->state) {
        case ATTACK:
            set_env_attack_mod(e, a_time_mod);
            e->s_mod = s_mod;
        case DECAY:
            set_env_decay_mod(e, d_time_mod);
            e->s_mod = s_mod;
        case SUSTAIN:
        case RELEASE:
            set_env_release_mod(e, r_time_mod);
    }
}

void process_env_r(struct env *e, bool amp) {
    e->level -= e->r_mod;

    if (e->level < 0.0) {
        e->level = 0.0;
    }
    
    //e->time++;
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
            e->level += e->a_mod;
            if (e->level > 1.0) {
                e->level = 1.0;
                e->state = DECAY;
            }
            break;
        case DECAY:
            e->level -= e->d_mod;
            if (e->level < e->s_mod) {
                e->level = e->s_mod;
                e->state = DECAY;
            }
            break;
        case SUSTAIN:
            // shouldn't actually be needed
            e->level = e->s_mod;
            break;
    }

    /*
    if (e->time < e->a_time) {
        e->mod += e->a_mod;
        e->state = ATTACK;
    } else if (e->time < (e->a_time + e->d_time)) {
        // the decay is starting from where the attack is, but it's still going up
        e->mod -= e->d_mod;
        e->state = DECAY;
    } else {
        // sustain should be at correct level after decay
        // do sustain 
        //e->mod = e->s_mod;
        e->state = SUSTAIN;
    }

    if (e->mod > 1.0) {
        e->mod = 1.0;
    }

    e->time++;
    */
}