#include "picosyn.h"

#ifndef TABLES_H
#define TABLES_H

// it's actually good if the table length is 256
#define WAVETABLE_LENGTH 1024
#define WAVETABLE_SCALE 22 // 32 - log2(wavetable length)

// 128 midi notes + 1 for detune, and 100 cents/semitone
#define INCREMENT_TABLE_LENGTH 12900 

extern const fixed SIN_TABLE[1024];
extern const fixed SAW_TABLE[1024];
extern const fixed TRI_TABLE[1024];
extern const fixed SQUARE_TABLE[1024];

extern const uint32_t INCREMENT_TABLE[12900];
extern const uint32_t KEYTRACK_TABLE[128];


#endif