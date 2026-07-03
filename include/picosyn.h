#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef PICOSYN_H
#define PICOSYN_H

#define ERR_LED_PIN 25
#define MIDI_LED_PIN 15
#define AUDIO_PIN 2

#define IO_EXPANDER_INT_PIN 23
#define MUX_A_PIN 17
#define MUX_B_PIN 19
#define MUX_C_PIN 20

#define VOICE_COUNT 8
#define GAIN 10//8192 // 2^(16-3)

#define SAMPLE_RATE 44100

// fixed type is used to avoid incorrect usage. it is a decimal type with one sign bit 
// and 31 fractional bits, i.e q31
// the fixed type should be used to represent fractions between -1.0 and 1.0

// rpi pico is little endian
typedef int32_t fixed;
#define FIXED_MAX INT32_MAX
#define FIXED_MIN INT32_MIN

static inline fixed mul_fixed(fixed a, fixed b) {
    int64_t tmp = ((int64_t)a * (int64_t) b);
    return (fixed)(tmp >> 31);
}
static inline fixed div_fixed(fixed a, fixed b) {
    int64_t tmp = (int64_t)(a << 31);
    return (fixed)(tmp / b);
}
static inline fixed float_to_fixed(float x) {
    // float should be between -1.0 and 1.0
    return (fixed)(x * FIXED_MAX);
}
static inline float fixed_to_float(fixed x) {
    // float will be between -1.0 and 1.0
    return ((float)x / FIXED_MAX);
}
static inline fixed byte_to_fixed(uint8_t x) {
    // scales so value is centered around 0ish.
    return (int64_t)x * 16843009 - FIXED_MAX;
}




/*
features when all finished:

2x osc. for each:
- selectable waveforms: sin, tri, saw, square, noise
- changeable pitch !
- supersaw
- (pwm)
together (modutlation)
- sync
- ring mod
- fm (osc 1 will modulate osc 2. knob will change modifier)

2x lfo. for each:
- rate
- amount
- as many mod destinations as possible
- (sync to clock maybe)

3x env. for each:
- attack
- decay
- sustain
- release
- for amp, filter and general modulation
modulation:
- env amount (for mod and filter)
- as many sources as possible for mod env. same as lfo if possible

filter.
- selecatble variants: lowpass, highpass and bandpass
- cutoff 
- resonance
- (drive)
- (key tracking) !

arpeggiator.
- midi sync bpm or button
- hold
- up, down, random (converge and diverge)
- (gate)

legato 
portamento 

presets.

*/

float pot_mod;
float pot_divider;

#endif