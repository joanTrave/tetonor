#ifndef TETONOR_SEEDED_RANDOM_H
#define TETONOR_SEEDED_RANDOM_H

#include <stdint.h>

/* A portable pseudo-random stream: the same seed gives the same values on
 * every platform (SplitMix64), unlike rand(). A plain value; nothing to free.
 */
typedef struct
{
    uint64_t state;
} SeededRandom;

/* Starts the stream at seed (state = seed). */
void init_seeded_random(SeededRandom* random, uint32_t seed);

/* The next 64-bit value (SplitMix64 step). */
uint64_t next_seeded_random(SeededRandom* random);

/* A value in [0, bound): (int)((next >> 32) % bound). Returns 0, still
 * advancing the stream, when bound < 1. */
int seeded_random_below(SeededRandom* random, int bound);

#endif
