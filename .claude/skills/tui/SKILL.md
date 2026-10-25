# TUI (curses) Skill

## Purpose

Define how Tetonor's terminal UI is written so it behaves the same in any terminal on Linux, macOS
and Windows, and stays separate from the game rules.

## Library and portability

- Target the X/Open (XSI) Curses API, which both **ncurses** (Linux/macOS) and **PDCurses /
  PDCursesMod** (Windows) implement. That shared subset is the contract.
- Include `<curses.h>`, not `<ncurses.h>` — every implementation ships `curses.h`; only ncurses
  ships `ncurses.h`.
- The link flag is a Makefile variable (e.g. `CURSES_LIB ?= -lncurses`, overridden with
  `-lpdcurses` on Windows), never hardcoded per source file.
- Anything the man pages mark as an ncurses *extension* (e.g. `set_escdelay`, `resizeterm`,
  `use_default_colors`) must be guarded with `#ifdef NCURSES_VERSION` (or a feature macro the
  implementation defines) and have a sensible fallback — or not be used.
- Use `ACS_*` line-drawing characters for boxes and borders; they map correctly on every backend.
  Keep text ASCII. UTF-8 literals or emoji need wide-character curses and are out of scope unless a
  task explicitly brings them in.
- Never emit raw ANSI escape sequences or call OS console APIs. curses owns the terminal.

## Layering

- Curses calls appear **only** under `src/ui/`. `src/game/` never includes `curses.h`.
- The UI talks to the game only through the port (`GamePort` in `src/game/game_port.h`). It never
  reaches into game internals or re-implements a rule (validation, scoring, generation).
- Layout maths (cell positions, centring, minimum size) lives in plain functions that take numbers
  and return numbers, so it can be unit tested without a terminal.

## Lifecycle

- `initscr()` is called exactly **once** per process and `endwin()` exactly once before exit, on
  every exit path (normal, ESC, error). Screens/menus do not call `initscr()` themselves; they
  receive an already-initialised terminal.
- Standard setup: `cbreak()`, `noecho()`, `keypad(stdscr, TRUE)` (and `keypad(win, TRUE)` for each
  window that reads input), `curs_set(0)` when the cursor is not needed (check its return value — it
  can fail on some terminals).
- Every `newwin`/`subwin` has a matching `delwin` owned by the same function or module that
  created it.
- Never write to `stdout`/`stderr` while curses is active.

## Input

- Store keys in `int`, never `char`. Compare against named constants: `KEY_UP`, `KEY_ENTER`,
  `KEY_RESIZE`, and project constants such as `TETONOR_KEY_ESCAPE` (27) and `'\n'`/`'\r'`
  (terminals differ on which Enter sends — accept both alongside `KEY_ENTER`).
- ESC is ambiguous with escape-sequence prefixes. Keep the default escape delay unless a task
  requires otherwise, and if it is lowered, do it through a guarded ncurses extension.
- Each screen has one input loop that maps keys to an action/result `enum` and returns it; the
  caller decides what happens next. No game state changes inside key-reading helpers.
- Ignore unknown keys silently — never crash or redraw garbage on unexpected input.

## Rendering

- Redraw from state: a screen's draw function takes the current state and paints it completely.
  Do not patch individual cells across frames from scattered places.
- Use `erase()`/`werase()` + draw + `refresh()`; avoid `clear()` in loops (forces a full repaint and
  flickers). With several windows, use `wnoutrefresh` for each, then one `doupdate()`.
- Check the terminal size with `getmaxyx` before drawing. If it is smaller than the layout's
  minimum, show a short "please enlarge the terminal" message instead of drawing off-screen.
- Handle `KEY_RESIZE` by recomputing the layout and redrawing.
- Colours are optional: call `has_colors()` before `start_color()`, define colour pairs in one named
  `enum`, and never encode meaning by colour alone (also use a marker, bold or reverse video).

## Review checklist

- No curses include or call outside `src/ui/`; no game rules inside `src/ui/`?
- `<curses.h>` used; any ncurses-only function guarded?
- `initscr`/`endwin` once each, `endwin` on every exit path; every window deleted?
- Keys held in `int` and compared against named constants?
- Small terminal and `KEY_RESIZE` handled?
- Works without colour support?
