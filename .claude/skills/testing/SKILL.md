# Testing Skill

## Purpose

Define reliable, portable verification for Tetonor's C code.

## Current state

The repository has **no test suite or `test`/`coverage` Makefile targets yet**. The first task that
adds tests must also add the infrastructure below (`tests/`, the harness header, and the `test` and
`coverage` targets). Until it exists, any agent that cannot run tests must say so — the Reviewer
reports `blocked`, never `approved`.

## Harness

- No third-party test framework: a single header `tests/test_harness.h` with a few assertion macros
  (`ASSERT_TRUE`, `ASSERT_EQ_INT`, `ASSERT_NOT_NULL`) that record failures with `__FILE__`/`__LINE__`
  and a runner that returns non-zero when any test failed. Pure C11, builds everywhere the game
  builds.
- Each test file has a `main` that runs its `test_*` functions; `make test` builds and runs every
  test binary and fails if any exits non-zero.
- Tests compile with the same flags as the game (`-std=c11 -Wall -Wextra -Werror -pedantic`).

## Rules

- Every new behaviour in `src/game/` has unit tests. The game core is the primary test target.
- Tests never link curses and never need a terminal. UI code is tested only through pure layout
  functions (positions, sizes, centring) that take and return numbers.
- Tests are deterministic: seed with a fixed value (`srand(42)`) or inject a stub random source —
  never depend on `time(NULL)`. When asserting on generated puzzles, assert **invariants** (every
  sum/product in the board is derivable from a pair of available numbers, the expected number of
  cells are hidden, values are within range) rather than exact random values.
- Test failure paths, not only happy paths: invalid input, boundary values (0, max value, full board,
  empty board), allocation failure where the code handles it.
- Every allocation made in a test is released; leaks in tests hide leaks in code.
- Test names describe the scenario: `test_<unit>_<scenario>`.
- No top-of-file comment block restating "tests for X", and no per-test comment that only restates
  the test name. Keep a comment only when it explains something non-obvious (why a seed or magic
  value was chosen, a subtle invariant).

## Test organisation

Mirror the source tree:

```text
tests/
  test_harness.h
  game/
    test_number_generation.c
    test_game_port.c
  ui/
    test_layout.c        # pure layout maths only
```

## Memory checking

Where available, run the tests under a sanitizer build:

```bash
make clean && make test CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic -g -fsanitize=address,undefined"
```

(AddressSanitizer is unavailable on some Windows toolchains — note that in the report rather than
skipping silently. `valgrind` is an acceptable alternative on Linux.)

## Coverage

Build tests with coverage instrumentation and summarise with `gcovr` (works with clang via
`llvm-cov gcov`):

```bash
make clean && make test CFLAGS="-std=c11 -Wall -Wextra -Werror -pedantic --coverage"
gcovr --gcov-executable "llvm-cov gcov" -r . --filter src/ --txt
```

The Reviewer factors per-file/per-line coverage into every approval decision, not only whether the
tests pass. Generated coverage files (`*.gcda`, `*.gcno`, `coverage.xml`) must not be committed.

## Diff coverage gate (reviewer, mandatory)

New/changed code in `src/game/` (and any pure function in `src/ui/`) must have at least 90% diff
coverage — coverage of the lines the change touches, not the project total:

```bash
gcovr --gcov-executable "llvm-cov gcov" -r . --filter src/ --cobertura coverage.xml
diff-cover coverage.xml --compare-branch=origin/main --fail-under=90
```

**Known exemption:** curses-drawing code in `src/ui/` (functions that call curses and cannot run
without a terminal) and `src/main.c` cannot be exercised by unit tests. Lines there do not count
against the gate on their own; instead the Reviewer verifies that any logic in those lines was
extracted into a testable function, and checks the UI behaviour manually or by reading the code.
Any other uncovered line is a real gap and needs a test before approval.

Regenerate `coverage.xml` fresh for each review; never reuse a stale one.

## Lint and format compliance

Whoever writes or edits a test file is responsible for its compliance: it must build with `-Werror`
and pass `clang-format --dry-run --Werror` with the repository `.clang-format`. Run
`clang-format -i` on every test file you add or modify.

## Completion checklist

- Happy path covered.
- Boundary and error cases covered.
- Tests deterministic (fixed seed or stub RNG) and terminal-free.
- `make test` executed and result reported (or its absence reported).
- Coverage and diff coverage measured; any genuine gap flagged.
- Every touched file builds warning-free and is `clang-format` clean.
