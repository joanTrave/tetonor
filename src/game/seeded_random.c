#include "seeded_random.h"

#define SPLITMIX_INCREMENT UINT64_C(0x9E3779B97F4A7C15)
#define SPLITMIX_MULTIPLIER_1 UINT64_C(0xBF58476D1CE4E5B9)
#define SPLITMIX_MULTIPLIER_2 UINT64_C(0x94D049BB133111EB)
#define SPLITMIX_SHIFT_1 30
#define SPLITMIX_SHIFT_2 27
#define SPLITMIX_SHIFT_3 31
#define UPPER_HALF_SHIFT 32

void init_seeded_random(SeededRandom* random, uint32_t seed)
{
    random->state = seed;
}

uint64_t next_seeded_random(SeededRandom* random)
{
    random->state += SPLITMIX_INCREMENT;
    uint64_t z = random->state;
    z          = (z ^ (z >> SPLITMIX_SHIFT_1)) * SPLITMIX_MULTIPLIER_1;
    z          = (z ^ (z >> SPLITMIX_SHIFT_2)) * SPLITMIX_MULTIPLIER_2;
    return z ^ (z >> SPLITMIX_SHIFT_3);
}

int seeded_random_below(SeededRandom* random, int bound)
{
    uint64_t next = next_seeded_random(random);
    if (bound < 1)
    {
        return 0;
    }
    return (int)((next >> UPPER_HALF_SHIFT) % (uint64_t)bound);
}
