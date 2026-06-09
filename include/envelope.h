#include "picosyn.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef ENVELOPE_H
#define ENVELOPE_H

extern const float ENV_MAX_TIME;
extern const float ENV_MAX_TIME_MOD;

typedef enum {
    LIN, EXP,
} env_mode;

typedef enum {
    ATTACK, DECAY, SUSTAIN, RELEASE,
} env_state;

//struct env {
//    fixed level;
//    env_state state;
//    env_mode mode;
//
//    fixed s_mod;
//    fixed a_mod;
//    fixed d_mod;
//    fixed r_mod;
//};

struct env {
    float level;
    env_state state;
    env_mode mode;

    float a_time;
    float d_time;
    float r_time;
   
    float s_mod;
    float a_mod;
    float d_mod;
    float r_mod;
};

void initialize_env(struct env *e, env_mode mode, float level);
void process_env_r(struct env *e);
void process_env_ads(struct env *e);
float get_env_level(struct env *e);

// these set_env_mod functions are technically not needed if it's updated every cycle
void set_env_sustain_mod(struct env *e, float s_mod);
void set_env_attack_mod(struct env *e, float a_time_mod);
void set_env_decay_mod(struct env *e, float d_time_mod);
void set_env_release_mod(struct env *e, float r_time_mod);

void update_env_a(struct env *e, float mod);
void update_env_d(struct env *e, float mod);
void update_env_r(struct env *e, float mod);
void update_env_s(struct env *e, float mod);

#endif

