# Architecture Skill

## Purpose

Apply a pragmatic ports-and-adapters architecture to a small C game: the game rules are a pure,
portable core; the terminal UI is an adapter that drives it through an explicit port.

## Layers

```text
src/
  main.c          # composition root: seed RNG, start the UI. No game rules, no drawing.
  game/           # core: puzzle generation, board state, move validation, win detection.
                  #   Standard C11 only. No curses, no I/O, no OS headers.
    game_port.h   # the port: the only surface the UI may use to talk to the game.
  ui/             # adapter: curses screens, menus, input loops, layout.
                  #   Talks to the game only via GamePort.
  [platform/]     # only if ever needed: OS-specific code behind a neutral header.
tests/            # unit tests for game/ (and pure layout maths from ui/).
```

## Dependency direction

`main.c → ui/ → game/game_port.h → game/*`. Never the reverse.

- `game/` includes nothing from `ui/` and no third-party header.
- `ui/` includes `game/game_port.h` (and the data types it exposes), not other `game/` internals.
- `main.c` wires things together; it is the only place allowed to know about both sides at once.

## Rules

- Game rules live in `game/`. If a UI change needs a rule (is this move legal? is the puzzle
  solved?), add it to the core and expose it through the port — do not compute it in the UI.
- The port (`GamePort`) is a struct of function pointers plus configuration. Add a pointer only when
  the UI genuinely needs a new capability; keep it the narrowest interface that works.
- Core functions are deterministic given their inputs and random source, so they are testable
  without a terminal.
- Ownership crosses the port explicitly: whatever the port returns is released through a matching
  port function (e.g. `generate_random_numbers_f` / `free_number_generation_f`).
- Avoid premature abstraction: one implementation behind the port is fine; do not add a second
  layer of indirection without a second real consumer (e.g. tests or another frontend).
- No hidden globals. State is held in structs passed explicitly.
- Record significant decisions (new module, new port function, data-structure change) in the
  architecture gate report.

## Review checklist

- Is dependency direction `ui → port → game` respected (check `#include` lines)?
- Is every game rule in `game/`, with none duplicated in `ui/`?
- Does `game/` build with only the C standard library?
- Is the port change minimal and are ownership/release functions paired?
- Can the changed core behaviour be tested without curses?
