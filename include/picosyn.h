#include <stdint.h>
#include <stdbool.h>

#ifndef PICOSYN_H
#define PICOSYN_H

#define ERR_LED_PIN 25
#define MIDI_LED_PIN 15
#define AUDIO_PIN 2

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

inline fixed mul_fixed(fixed a, fixed b) {
    int64_t temp = (int64_t)(a * b);
    return (fixed)temp >> 31;
}
inline fixed div_fixed(fixed a, fixed b) {
    int64_t temp = (int64_t)(a << 31);
    return (fixed)(temp / b);
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