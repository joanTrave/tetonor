#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../test_harness.h"
#include "../../src/game/seeded_random.h"

#define TEST_REFERENCE_DRAW_COUNT 3
#define TEST_LONG_RUN_COUNT 100
#define TEST_MIN_BOUND 1
#define TEST_MAX_BOUND 30
#define TEST_BOUND_SEED_COUNT 5
#define TEST_BOUND_DRAW_COUNT 50
#define TEST_SMALL_BOUND 7
#define TEST_EXPECTED_BELOW_SEED_0_BOUND_7 4
#define TEST_RAND_SEED 1234

/* Fails at the first draw that does not equal expected[i]; a PRNG's own
 * output has no partial-credit notion. */
static void assert_sequence_matches(uint32_t seed, const uint64_t* expected,
                                    size_t count)
{
    SeededRandom random;
    init_seeded_random(&random, seed);
    for (size_t i = 0; i < count; i++)
    {
        ASSERT_TRUE(next_seeded_random(&random) == expected[i]);
    }
}

static void
test_next_seeded_random_matches_splitmix64_reference_for_seed_0(void)
{
    static const uint64_t expected[TEST_REFERENCE_DRAW_COUNT] = {
        UINT64_C(16294208416658607535), UINT64_C(7960286522194355700),
        UINT64_C(487617019471545679)};
    assert_sequence_matches(0, expected, TEST_REFERENCE_DRAW_COUNT);
}

static void
test_next_seeded_random_matches_splitmix64_reference_for_seed_1234567(void)
{
    static const uint64_t expected[TEST_REFERENCE_DRAW_COUNT] = {
        UINT64_C(6457827717110365317), UINT64_C(3203168211198807973),
        UINT64_C(9817491932198370423)};
    assert_sequence_matches(1234567, expected, TEST_REFERENCE_DRAW_COUNT);
}

static void test_next_seeded_random_for_seed_20261027(void)
{
    static const uint64_t expected[TEST_REFERENCE_DRAW_COUNT] = {
        UINT64_C(7671909092089349006), UINT64_C(7159423796239308355),
        UINT64_C(16197453062806911444)};
    assert_sequence_matches(20261027, expected, TEST_REFERENCE_DRAW_COUNT);
}

static void test_init_seeded_random_sets_state_to_seed(void)
{
    SeededRandom random;
    init_seeded_random(&random, 123456789);
    ASSERT_TRUE(random.state == UINT64_C(123456789));
}

static void test_next_seeded_random_same_seed_repeats(void)
{
    SeededRandom first;
    SeededRandom second;
    init_seeded_random(&first, 99);
    init_seeded_random(&second, 99);
    bool all_equal = true;
    for (int i = 0; i < TEST_LONG_RUN_COUNT; i++)
    {
        if (next_seeded_random(&first) != next_seeded_random(&second))
        {
            all_equal = false;
        }
    }
    ASSERT_TRUE(all_equal);
}

static void test_next_seeded_random_different_seeds_diverge(void)
{
    SeededRandom first;
    SeededRandom second;
    init_seeded_random(&first, 1);
    init_seeded_random(&second, 2);
    bool any_different = false;
    for (int i = 0; i < TEST_LONG_RUN_COUNT; i++)
    {
        if (next_seeded_random(&first) != next_seeded_random(&second))
        {
            any_different = true;
        }
    }
    ASSERT_TRUE(any_different);
}

static void test_seeded_random_below_matches_the_shifted_draw(void)
{
    SeededRandom random;
    init_seeded_random(&random, 0);
    int value = seeded_random_below(&random, TEST_SMALL_BOUND);
    ASSERT_EQ_INT(value, TEST_EXPECTED_BELOW_SEED_0_BOUND_7);
}

static void test_seeded_random_below_stays_within_bounds(void)
{
    bool all_in_range = true;
    for (int bound = TEST_MIN_BOUND; bound <= TEST_MAX_BOUND; bound++)
    {
        for (uint32_t seed = 1; seed <= TEST_BOUND_SEED_COUNT; seed++)
        {
            SeededRandom random;
            init_seeded_random(&random, seed);
            for (int draw = 0; draw < TEST_BOUND_DRAW_COUNT; draw++)
            {
                int value = seeded_random_below(&random, bound);
                if (value < 0 || value >= bound)
                {
                    all_in_range = false;
                }
            }
        }
    }
    ASSERT_TRUE(all_in_range);
}

static void test_seeded_random_below_bound_one_is_always_zero(void)
{
    SeededRandom random;
    init_seeded_random(&random, 42);
    bool all_zero = true;
    for (int i = 0; i < TEST_LONG_RUN_COUNT; i++)
    {
        if (seeded_random_below(&random, 1) != 0)
        {
            all_zero = false;
        }
    }
    ASSERT_TRUE(all_zero);
}

static void test_seeded_random_below_non_positive_bound_is_zero(void)
{
    const int bounds[] = {0, -1, -30};
    for (size_t i = 0; i < sizeof bounds / sizeof bounds[0]; i++)
    {
        SeededRandom random;
        init_seeded_random(&random, 7);
        ASSERT_EQ_INT(seeded_random_below(&random, bounds[i]), 0);
    }
}

/* A non-positive bound still steps the stream once, so it cannot be used to
 * peek without consuming a draw. */
static void test_seeded_random_below_non_positive_bound_still_advances(void)
{
    SeededRandom consumed;
    SeededRandom skipped;
    init_seeded_random(&consumed, 7);
    init_seeded_random(&skipped, 7);
    seeded_random_below(&consumed, 0);
    uint64_t after_consumed = next_seeded_random(&consumed);
    next_seeded_random(&skipped); /* discarded: mirrors the call above */
    uint64_t after_skipped = next_seeded_random(&skipped);
    ASSERT_TRUE(after_consumed == after_skipped);
}

static void test_seeded_random_does_not_touch_rand(void)
{
    srand(TEST_RAND_SEED);
    int reference = rand();
    srand(TEST_RAND_SEED);
    SeededRandom random;
    init_seeded_random(&random, 55);
    for (int i = 0; i < TEST_LONG_RUN_COUNT; i++)
    {
        seeded_random_below(&random, TEST_MAX_BOUND);
    }
    int after = rand();
    ASSERT_EQ_INT(after, reference);
}

int main(void)
{
    RUN_TEST(test_next_seeded_random_matches_splitmix64_reference_for_seed_0);
    RUN_TEST(
        test_next_seeded_random_matches_splitmix64_reference_for_seed_1234567);
    RUN_TEST(test_next_seeded_random_for_seed_20261027);
    RUN_TEST(test_init_seeded_random_sets_state_to_seed);
    RUN_TEST(test_next_seeded_random_same_seed_repeats);
    RUN_TEST(test_next_seeded_random_different_seeds_diverge);
    RUN_TEST(test_seeded_random_below_matches_the_shifted_draw);
    RUN_TEST(test_seeded_random_below_stays_within_bounds);
    RUN_TEST(test_seeded_random_below_bound_one_is_always_zero);
    RUN_TEST(test_seeded_random_below_non_positive_bound_is_zero);
    RUN_TEST(test_seeded_random_below_non_positive_bound_still_advances);
    RUN_TEST(test_seeded_random_does_not_touch_rand);
    return test_harness_result();
}
