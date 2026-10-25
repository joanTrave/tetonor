# C Skill

## Purpose

Define the C implementation conventions for Tetonor: portable, warning-free, memory-safe C11 that
builds with the same flags on Linux, macOS and Windows.

## Baseline

- Language: ISO C11 (`-std=c11`). No GNU extensions, no compiler-specific attributes or builtins
  unless wrapped in a portability macro in a single header.
- Compiler: clang (`CC=clang` in `Makefile`). Code must also compile cleanly with gcc and with
  clang targeting MinGW/Windows — do not rely on clang-only behaviour.
- Warnings: `-Wall -Wextra -Werror -pedantic`. Warnings are errors; never silence one with a
  pragma or by dropping a flag — fix the code.
- Build: `make` (see `Makefile`). `make clean && make` must succeed from a clean tree.
- Format: `clang-format` with the repository's `.clang-format` (LLVM base, Allman braces, 4-space
  indent, 80 columns, `int* p` pointer alignment, `SortIncludes: false`).
- TUI library: curses — see `.claude/skills/tui/SKILL.md`.

## Portability rules (OS independence)

- Only the C11 standard library in `src/game/`. No `<unistd.h>`, `<sys/*.h>`, `<termios.h>`,
  `<windows.h>`, `<pthread.h>`, `<dirent.h>` anywhere in the game core.
- Platform-specific code, if it ever becomes unavoidable, lives in one file under `src/platform/`
  behind a neutral header, selected with `#if defined(_WIN32)` / `#else`. Never sprinkle `#ifdef`
  through game or UI logic.
- Do not assume `RAND_MAX` (it is `32767` on Windows). Do not assume `int` is 32 bits for values
  that could overflow — use `<stdint.h>` fixed-width types where size matters.
- Do not assume `char` signedness. Use `int` for values returned by `getch()`/`wgetch()`.
- Use `size_t` for sizes and indices that come from `sizeof`/allocations; cast explicitly when
  mixing with `int`.
- No VLAs (optional in C11 and unsupported by MSVC-compatible toolchains).

## Headers

- Every header has an include guard `TETONOR_<PATH>_<NAME>_H` matching its path
  (`src/game/game_port.h` → `TETONOR_GAME_PORT_H`).
- A header includes exactly what its declarations need — nothing more. A `.c` file includes its own
  header first among project headers, so a missing include in the header is caught.
- Include order: standard library, then third-party (`<curses.h>`), then project headers, each group
  separated by a blank line. `SortIncludes` is off, so keep this order by hand.
- Project includes are relative to the including file (`"../game/game_port.h"`), matching the
  existing code.
- Headers declare only the module's public API. Private helpers are never declared in headers.

## Linkage and visibility

- Every function or file-scope variable not in the module's header is `static`.
- Do not name identifiers with a leading underscore. Identifiers beginning with `_` at file scope
  (and `_` + uppercase or `__` anywhere) are reserved by the C standard (C11 §7.1.3). Use `static`
  for privacy instead of a `_` prefix.
- No mutable global state shared between modules. Pass state explicitly (structs, ports).

## Memory management

- Every allocation has exactly one documented owner and one matching release function
  (`create_x`/`destroy_x`, `generate_x`/`free_x`). The pair lives in the same module.
- Check every `malloc`/`calloc`/`realloc` result. On failure, release what was already allocated in
  that function and return `NULL` (or an error code) — never dereference an unchecked pointer.
- Prefer `calloc` or explicit initialisation so no field is ever read uninitialised.
- Write `malloc(sizeof *ptr)` / `malloc(n * sizeof *ptr)` rather than repeating the type.
- `realloc` into a temporary, never directly into the only pointer to the block.
- Release functions accept `NULL` and do nothing, like `free`.
- Prefer fixed-size arrays inside structs for fixed-size game data (a 4×4 board is `int [4][4]`,
  not `int**`) — fewer allocations, fewer failure paths, nothing to leak.
- Every early-return path frees what the function owns. Use a single cleanup label
  (`goto cleanup;`) when a function owns more than one resource.

## Constants and types

- No magic numbers. Board dimensions, value ranges, sentinels and key codes are named
  (`#define TETONOR_GRID_SIZE 4`, `enum { KEY_ESCAPE = 27 };`) in the header of the module that
  owns the concept.
- Sentinels (e.g. "hidden number") are named constants, never a bare `-1` repeated in several files.
- Use `enum` for closed sets of states/results, `bool` from `<stdbool.h>` for flags.
- `typedef struct { ... } Name;` is the existing style for module types; keep it.
- Mark pointer parameters `const` when the function does not modify the pointee.

## Error handling

- Functions that can fail return a status (`bool`, an `enum` result, or `NULL`) and the caller
  checks it. No silent failures, no `exit()` from library code — only `main` decides to exit.
- Never print diagnostics with `printf`/`fprintf(stderr, ...)` while curses is active; it corrupts
  the screen. Propagate the error up to where the UI can show it or curses has been ended.

## Randomness

- Seed once in `main` (`srand((unsigned)time(NULL))`, as today). Game logic must not call `srand`.
- Game-core functions that need randomness take a random source (function pointer or seed) so tests
  can make them deterministic.
- Use the unbiased Fisher–Yates shuffle: `j = i + rand() % (n - i)`.

## Code quality

- Functions do one thing and stay short; split anything that needs a comment to explain its phases.
- Keep cyclomatic complexity low; prefer early returns over deep nesting.
- Comments explain *why*, not *what*. `/* ... */` and `//` are both C11; follow the surrounding file.
- No dead code, no commented-out code, no `TODO` without a task id.

## Verification commands

```bash
make clean && make                                     # must build with zero warnings (-Werror)
clang-format --dry-run --Werror src/**/*.c src/**/*.h  # formatting check
make test                                              # see .claude/skills/testing/SKILL.md
```

Run `clang-format -i <file>` on every file you add or modify. If a tool is not installed in the
environment, say so explicitly in your report — never claim a check passed that was not run.

## Review checklist

- Builds with `-std=c11 -Wall -Wextra -Werror -pedantic` and no new warnings?
- Only standard C in `src/game/`, curses only in `src/ui/`, no OS headers?
- Every allocation checked and paired with a release on every path?
- Private functions `static`, no leading-underscore identifiers?
- No magic numbers; sentinels and dimensions named once?
- `clang-format` clean?
