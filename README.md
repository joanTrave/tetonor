# Tetonor (Terminal Edition)

A terminal-based implementation of the Tetonor puzzle game, inspired by the original Sunday Times editions.

## Tech stack
- C (C11)
- clang
- curses (ncurses on Linux/macOS, PDCurses on Windows)

## Requirements

| Tool | Used for |
|---|---|
| `clang` | compiler (`-std=c11 -Wall -Wextra -Werror -pedantic`) |
| `make` (with a POSIX shell) | build, test and check targets |
| curses library | ncurses (Linux/macOS) or PDCurses (Windows) |
| `clang-format` | `make format-check` |
| `llvm-cov` | `make coverage` (gcov-compatible reports) |
| Python 3 with `gcovr` and `diff-cover` | `make coverage`, `make diff-coverage` |

## Install

### Windows (MSYS2 UCRT64)

Install MSYS2, then use its **UCRT64** environment for everything below.

```powershell
winget install --id MSYS2.MSYS2 --exact --source winget --accept-package-agreements --accept-source-agreements
```

In an MSYS2 UCRT64 shell (run the update twice; the first run may close the shell):

```bash
pacman -Syu --noconfirm
pacman -S --needed --noconfirm make \
    mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-clang-tools-extra \
    mingw-w64-ucrt-x86_64-llvm mingw-w64-ucrt-x86_64-compiler-rt \
    mingw-w64-ucrt-x86_64-pdcurses mingw-w64-ucrt-x86_64-python \
    mingw-w64-ucrt-x86_64-python-pip mingw-w64-ucrt-x86_64-python-lxml
python -m venv --system-site-packages ~/.venvs/tetonor-tools
~/.venvs/tetonor-tools/bin/python -m pip install --upgrade pip gcovr diff-cover
export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"   # add to ~/.bashrc
```

Do not install the MSYS2 `ncurses` package: its `curses.h` would shadow PDCurses.
The venv lives outside the repository and reuses the pacman-built `lxml`.

Run the tools from a UCRT64 shell, or from PowerShell through MSYS2's bash:

```powershell
$env:MSYSTEM='UCRT64'; $env:CHERE_INVOKING='1'; $env:MSYS2_PATH_TYPE='inherit'
& 'C:\msys64\usr\bin\bash.exe' -lc 'export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"; make test'
```

### Linux (Debian/Ubuntu)

```bash
sudo apt install clang clang-format llvm make libncurses-dev python3-venv
python3 -m venv ~/.venvs/tetonor-tools
~/.venvs/tetonor-tools/bin/pip install gcovr diff-cover
export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"
```

### macOS

```bash
xcode-select --install
brew install llvm clang-format python
python3 -m venv ~/.venvs/tetonor-tools
~/.venvs/tetonor-tools/bin/pip install gcovr diff-cover
export PATH="$HOME/.venvs/tetonor-tools/bin:$PATH"
```

## Build

```bash
make            # builds ./tetonor (./tetonor.exe on Windows)
make clean
```

Linux and macOS link `-lncurses`. On Windows the Makefile defaults to the PDCurses
console backend (`-lpdcurses_wincon -lwinmm`, with `-DPDC_FORCE_UTF8`). Override with
`make CURSES_LIB=... CURSES_CFLAGS=...` if your curses is installed elsewhere. Plain
`-lpdcurses` is the GUI backend and is not supported by the default setup.

## Test

```bash
make test       # builds and runs every tests/**/test_*.c (no curses, no terminal)
make sanitize   # same tests with -fsanitize=address,undefined
```

`SANITIZE_FLAGS` is overridable. AddressSanitizer does not link on MSYS2 UCRT64 (missing
clang runtime libraries); there use
`make sanitize SANITIZE_FLAGS="-g -fsanitize=undefined -fsanitize-trap=undefined"`.

## Coverage

```bash
make coverage        # clean rebuild with --coverage, runs tests, prints a gcovr summary, writes coverage.xml
make diff-coverage   # coverage, then fails if changed lines vs origin/main are below 90% covered
```

Variables: `COMPARE_BRANCH` (default `origin/main`), `DIFF_COVERAGE_THRESHOLD` (default `90`),
`GCOV` (default `llvm-cov gcov`; on macOS use `make coverage GCOV="xcrun llvm-cov gcov"`).
Curses files and `src/main.c` are not unit-tested and do not appear in the report.

## Format

```bash
make format-check   # clang-format --dry-run --Werror on src/ and tests/
```

## Run

```bash
./tetonor
```

SPACE or ENTER on the main menu opens the difficulty screen: UP/DOWN choose Easy, Medium (pre-selected) or Hard, SPACE or ENTER starts the game, ESC goes back to the main menu. ESC during a game returns to the main menu at once, without asking; ESC on the main menu quits. `S` (or `s`) on the main menu opens the statistics screen, and ESC goes back to the main menu. Each number of the matrix has three boxes under it: the smaller number, an operation and the greater number. Arrows move between all boxes: LEFT/RIGHT inside a matrix row, UP/DOWN between rows. DOWN from the bottom row goes to the available row, UP from it goes back to the boxes, and DOWN again goes to the CONFIRM button; UP or LEFT return from it. DOWN from CONFIRM goes to the RESTART button, UP returns to CONFIRM; ENTER or SPACE on RESTART starts a brand new game at once (no confirmation). Digits (main row, or numpad with NumLock on) type a number from 0 to 29, `+` sets addition, `x`, `X` or `*` set multiplication (always shown as `x`, also from the numpad), and BACKSPACE deletes. `U` (or `u`) undoes the last edit: a typed digit, a deleted digit or an operation, up to the last 20, one per press. It restores the boxes, their colours and the result as they were before that edit, and the cursor stays where it is. With nothing left to undo, "Nothing to undo" appears beside the result line until the next key. A new game starts with nothing to undo. ENTER or SPACE on CONFIRM judges the whole board and shows CORRECT ANSWER or WRONG ANSWER (nothing happens while a box is still empty): every pair of numbers must be used once added and once multiplied, and any valid solution wins. ENTER on a box does nothing. Editing a box clears a WRONG ANSWER; once CORRECT ANSWER is shown the board is locked: digits, operations, BACKSPACE, undo and CONFIRM do nothing, while the arrows, RESTART, `N`, `M`, `P` and ESC still work. A number in the available row turns blue when one operand box uses it and green once it is used in both an addition and a multiplication; numbers you typed use bright blue and bright green instead (on terminals with at least 16 colours); with several equal numbers, each box is coloured on its own. The terminal must be at least 80x27. On Windows run it in a real console such as
Windows Terminal or `cmd`/PowerShell, not in mintty (the default MSYS2 terminal), and put
`C:\msys64\ucrt64\bin` on `PATH` for the runtime DLLs (or build with `make LDFLAGS=-static`).

### Shortcuts and pause

A help area at the bottom of the game screen, left of the RESTART button, lists the controls. The game screen has these shortcuts (upper or lower case):

| Key | Action |
|---|---|
| `P` | Pause: opens the pause menu |
| `N` | New game at the same difficulty (fresh board, empty undo history, timer at zero) |
| `M` | Back to the main menu |
| `U` | Undo the last edit |
| ESC | Leave the game at once, without asking |

While the game is paused the board is hidden and the timer is frozen at the value it had; time spent paused is not counted, however long the pause lasts. No key changes the board and CONFIRM cannot happen while paused. The pause menu offers Resume and Abandon: UP/DOWN choose, SPACE or ENTER select, ESC or `P` resume, and the game continues exactly as it was (boxes, cursor, undo history and result).

Abandon leaves the game and shows the main menu, like `M`; nothing is recorded. `N`, `M` and Abandon ask before they act only when the game is not solved and has at least one edit to undo ("Progress in this game will be lost."). On the question, `Y` or Yes performs the action, `N`, No or ESC cancels and leaves the game unchanged (cancelling Abandon returns to the pause menu); the question opens on No, so ENTER right away cancels. A fresh game and a solved game (CORRECT ANSWER) never ask. The RESTART button and ESC never ask.

### Timer and statistics

A timer beside the board (`Time: MM:SS`, or `H:MM:SS` from one hour, capped at `99:59:59`) counts the game time from the moment the game starts, also while you are not pressing keys. It stops at the first CORRECT ANSWER and that game is recorded once; the board is then locked, so the game is recorded once and never again. RESTART starts a new game and the timer starts again at zero. Time spent paused is not counted. Leaving a game (ESC, `M` or Abandon), or quitting before CORRECT ANSWER, records nothing.

`S` on the main menu shows, per difficulty, the number of completed games, the best time and the average time (rounded to the nearest second, halves up). A level without completed games shows `--:--`.

### Statistics file

Statistics are saved after every completed game and loaded when the game starts. The file is `statistics.txt` in a per-user folder, created on the first completed game:

| OS | Path |
|---|---|
| Windows | `%LOCALAPPDATA%\Tetonor\statistics.txt` |
| Linux | `~/.local/share/tetonor/statistics.txt` |
| macOS | `~/Library/Application Support/Tetonor/statistics.txt` |

It is a small text file that starts with `TETONOR-STATS 1`. A missing file means empty statistics; an empty, truncated, edited or otherwise damaged file is ignored and replaced by the next completed game (no backup is kept). If the folder cannot be created or written, the game still runs and keeps its statistics for the session, but nothing is saved. Delete the file to reset the statistics.

### Difficulty

The level chosen before a game stays fixed for that game, and RESTART starts a new puzzle at the same level. The next game from the main menu asks again, with Medium pre-selected. No level uses the number 0.

| Level | Numbers in the puzzle | Hidden numbers in the available row | Pairs with both numbers hidden |
|---|---|---|---|
| Easy | 1 to 12 | 4 | 0 |
| Medium | 1 to 20 | 8 | 0 |
| Hard | 1 to 29 | 12 | 4 |

On Easy and Medium every hidden number has a visible partner from its own pair, so it can be found from one sum and one product. On Hard some pairs have both numbers hidden. These are structural differences that the tests check; how hard a level feels to a player is not measured. A puzzle may have more than one valid solution, and any valid solution wins. Because Easy uses a smaller range of numbers, unrelated matrix cells can match a sum or product by coincidence more often there than on Hard.
