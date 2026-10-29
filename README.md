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

SPACE or ENTER on the main menu opens the difficulty screen: UP/DOWN choose Easy, Medium (pre-selected) or Hard, SPACE or ENTER starts the game, ESC goes back to the main menu. ESC during a game returns to the main menu at once, without asking; ESC on the main menu quits. `S` (or `s`) on the main menu opens the statistics screen, and ESC goes back to the main menu. `T` (or `t`) opens the theme screen (see Themes). `C` (or `c`) on the main menu continues the saved game, when there is one (see "Save and continue"). `D` (or `d`) on the main menu starts the daily challenge (see "Daily challenge"). Each number of the matrix has three boxes under it: the smaller number, an operation and the greater number. Arrows move between all boxes: LEFT/RIGHT inside a matrix row, UP/DOWN between rows. DOWN from the bottom row goes to the available row, UP from it goes back to the boxes, and DOWN again goes to the CONFIRM button; UP or LEFT return from it. DOWN from CONFIRM goes to the RESTART button, UP returns to CONFIRM; ENTER or SPACE on RESTART starts a brand new game at once (no confirmation). Digits (main row, or numpad with NumLock on) type a number from 0 to 29, `+` sets addition, `x`, `X` or `*` set multiplication (always shown as `x`, also from the numpad), and BACKSPACE deletes. `U` (or `u`) undoes the last edit: a typed digit, a deleted digit or an operation, up to the last 20, one per press. It restores the boxes, their colours and the result as they were before that edit, and the cursor stays where it is. With nothing left to undo, "Nothing to undo" appears beside the result line until the next key. A new game starts with nothing to undo. `R` (or `r`) redoes the last undone edit, one per press, up to as many as were undone: it puts back the boxes, their colours and the result as they were before that undo, and the cursor stays where it is. Any new edit after an undo discards what could still be redone. With nothing to redo, "Nothing to redo" appears in the same place until the next key. A new or continued game starts with nothing to redo. ENTER or SPACE on CONFIRM judges the whole board and shows CORRECT ANSWER or WRONG ANSWER (nothing happens while a box is still empty): every pair of numbers must be used once added and once multiplied, and any valid solution wins. Solving the puzzle opens the victory screen (see below). ENTER on a box does nothing. Editing a box clears a WRONG ANSWER; once the answer is CORRECT the board is locked and replaced by the victory screen, where only its own keys work. A number in the available row turns blue when one operand box uses it and green once it is used in both an addition and a multiplication; numbers you typed use bright blue and bright green instead (on terminals with at least 16 colours); with several equal numbers, each box is coloured on its own. Boxes that already break a rule are marked while you type, before CONFIRM: a group is framed by `!` (`!29|x| 7!`) with the offending boxes in magenta (dark grey in the Light theme) when its numbers are out of order, when its result is not its matrix number, when a number is used in more additions or more multiplications than the available row can hold, or when a pair is used with one operation in more cells than the matrix allows; an available number you typed out of order is framed by `!` on its sides and keeps its blue or green colour. "Conflicts: N (marked !)" on the result line counts the marked groups and numbers, and goes away once none is left. A box is marked only when the rule is certainly broken, so an unfinished but still solvable board shows nothing, and empty boxes are never marked. Marks never block or change your input, and they work without colour thanks to the `!` frame. The terminal must be at least 80x27. On Windows run it in a real console such as
Windows Terminal or `cmd`/PowerShell, not in mintty (the default MSYS2 terminal), and put
`C:\msys64\ucrt64\bin` on `PATH` for the runtime DLLs (or build with `make LDFLAGS=-static`).

### Shortcuts and pause

A help area at the bottom of the game screen, left of the RESTART button, lists the controls. The game screen has these shortcuts (upper or lower case):

| Key | Action |
|---|---|
| `P` | Pause: opens the pause menu |
| `N` | New game at the same difficulty (fresh board, empty undo and redo history, timer at zero) |
| `M` | Back to the main menu |
| `U` | Undo the last edit |
| `R` | Redo the last undone edit |
| `H` | Show a hint (does not change the board; the game then no longer counts for statistics) |
| `T` | Choose a theme (see Themes) |
| ESC | Leave the game at once, without asking |

While the game is paused the board is hidden and the timer is frozen at the value it had; time spent paused is not counted, however long the pause lasts. No key changes the board and CONFIRM cannot happen while paused. The pause menu offers Resume and Abandon: UP/DOWN choose, SPACE or ENTER select, ESC or `P` resume, `S` saves the game (see below), and the game continues exactly as it was (boxes, cursor, undo and redo history and result).

Abandon leaves the game and shows the main menu, like `M`; nothing is recorded. `N`, `M` and Abandon ask before they act only when the game is not solved and has at least one edit to undo ("Progress in this game will be lost."). On the question, `Y` or Yes performs the action, `N`, No or ESC cancels and leaves the game unchanged (cancelling Abandon returns to the pause menu); the question opens on No, so ENTER right away cancels. A fresh game never asks, and the victory screen never asks. The RESTART button and ESC never ask.

### Save and continue

Press `P` to pause, then `S` (or `s`) in the pause menu to save the game; the pause menu shows "Game saved." and stays open, so you can resume, save again or abandon. `S` does nothing on the board itself. If the game cannot be written to disk but can be kept in memory, the menu shows "Saved for this session only." and the save works until you quit; if it cannot be saved at all, it shows "This game cannot be saved." and play goes on as before.

There is one saved game: a new save replaces the previous one. When a saved game exists, the main menu shows "Press C to continue the saved game" (upper or lower case `C`); without one the line is not shown and `C` does nothing. Continuing skips the difficulty screen and opens the game at its own difficulty.

A continued game comes back with its difficulty, the same puzzle, every box, the CORRECT/WRONG state of the last CONFIRM, the number of moves and the elapsed time. The undo and redo history and the cursor position are not saved: the undo and redo history start empty, and the cursor starts on the first box to fill, as in a new game. The time counts from the moment you continue, not while the program was closed or on a menu. Because the undo history starts empty, `N`, `M` and Abandon do not ask for confirmation right after continuing.

Continuing does not delete the save. It stays until you save again (which replaces it) or until you solve that same game, which removes it; leaving or abandoning a game never removes it. Starting another game (`N`, RESTART or New game) ends the link to the save, so solving that other game keeps the saved one. Saving, continuing and discarding never change the statistics; a continued game is recorded once, when you solve it, like any other, unless a hint was shown in it.

### Daily challenge

Press `D` (or `d`) on the main menu ("Press D for today's daily challenge") to play the daily challenge. It starts at once, without the difficulty screen. Everyone who plays on the same day gets the same puzzle, on any computer: the puzzle depends only on the date.

- The day is the UTC calendar day, not your local one, so the challenge changes at 00:00 UTC. The date is fixed when the challenge starts: a game started before midnight UTC stays on its own day, and the next `D` from the main menu gives the new day's puzzle.
- The difficulty rotates with the day of the month and cannot be chosen: day 1, 4, 7... is Medium, day 2, 5, 8... is Hard and day 3, 6, 9... is Easy (day modulo 3: 0 Easy, 1 Medium, 2 Hard).
- While you play, a highlighted "DAILY CHALLENGE YYYY-MM-DD" badge on the top line shows the date. The board works exactly like a normal game.
- A daily challenge is never saved: the pause menu shows "Daily challenges are not saved" and `S` does nothing. It never replaces or removes a saved game from "Save and continue", and `C` on the main menu still continues that saved game.
- It is not counted in the statistics, solved or not. Solving it opens the victory screen as usual.
- `N`, RESTART and New game on the victory screen start the same daily puzzle again from scratch, with the board and the timer reset. Leaving with `M`, ESC or Abandon loses your progress; play it again from `D`, as often as you like.
- Playing the daily challenge does not change the random games: the other modes behave exactly as before.

### Victory screen

When CONFIRM finds a correct answer the board is replaced by the "Puzzle solved!" screen. It summarises the finished game:

| Line | Content |
|---|---|
| `Time:` | The game time, in the same format as the timer (`MM:SS`, or `H:MM:SS` from one hour) and equal to the time recorded in the statistics |
| `Difficulty:` | Easy, Medium or Hard |
| `Moves:` | The number of moves of this game |

A move is an edit that changes a box: typing or deleting a digit, or setting or deleting an operation, when the box ends up different. Undo, redo, the arrows, CONFIRM and edits that change nothing are not moves, and undoing or redoing a move does not change the count. A new game starts again at 0.

The screen offers New game (a fresh game at the same difficulty) and Main menu: UP/DOWN choose, SPACE or ENTER select, `N` starts a new game, `M` or ESC goes to the main menu. Neither action asks for confirmation. Any other key does nothing.

### Themes

Two colour themes are available: Classic (the default, the dark look the game always had) and Light (black text on a light background, with colours chosen to stay readable on it). A theme covers the whole game: the board, every coloured number state, the menus, the other screens and the background.

Press `T` (or `t`) to choose a theme from the main menu, during a game, or on the pause menu. The theme screen lists the themes with the one in use marked "(current)": UP/DOWN move, SPACE or ENTER apply the highlighted theme at once (no restart), ESC or `T` go back without changing anything. During a game the board is hidden while the theme screen is open and shown again, redrawn in the chosen theme, when it closes. Choosing a theme changes nothing but the colours: the numbers you entered, the undo and redo history, the result and the statistics stay as they were. The timer is paused while the theme screen is open, like in the pause menu, so choosing a theme costs no game time.

On terminals with only 8 colours the themes use the closest basic colours, and without colour support both themes look the same (plain text with the usual bold and underline cues).

### Hints

Press `H` (or `h`) during a game to ask for a hint. The game looks for a complete valid answer that keeps every box you have already filled, and shows one thing to enter, under the timer: either an available number (`Hint: Available box 3`, `enter 7`) or a whole group of a matrix number (`Hint: row 2, column 4`, `enter 3 x 8`). The suggested box is framed by `?`. Hints start from the box under the cursor when it is an empty available box or an unfilled group, otherwise from the first unfilled group, then the first empty available box. Following the hints one after another solves the puzzle without ever creating a marked conflict.

Asking for a hint changes nothing on the board: no box, cursor, undo and redo history, move count, result or timer is touched, and the pause state stays as it was. The hint stays on screen until a move changes the boxes, you undo or redo a move, a new game starts or you ask for another hint; moving the cursor keeps it.

When there is no hint, the game says why: "No hint: puzzle already solved.", "No hint: fix the boxes marked !" (a marked conflict must be fixed first), "No hint: every box is filled." (press CONFIRM or clear a box) or "No hint for these entries." (no valid answer keeps them, so some may be wrong: try undo). These replies do not mark the game.

A shown hint marks the game: "Hint used: this game will not count for statistics." appears under the hint, and solving the game records nothing in the statistics. The timer runs as usual and the victory screen is unchanged. The mark is kept when you save and continue the game, and starting a new game clears it.

### Timer and statistics

A timer beside the board (`Time: MM:SS`, or `H:MM:SS` from one hour, capped at `99:59:59`) counts the game time from the moment the game starts, also while you are not pressing keys. It stops when the puzzle is solved; that game is recorded once and the victory screen shows the same time. RESTART starts a new game and the timer starts again at zero. Time spent paused is not counted. Leaving a game (ESC, `M` or Abandon), or quitting before CORRECT ANSWER, records nothing.

`S` on the main menu shows, per difficulty, the number of completed games, the best time and the average time (rounded to the nearest second, halves up). A level without completed games shows `--:--`.

### Statistics file

Statistics are saved after every completed game and loaded when the game starts. The file is `statistics.txt` in a per-user folder, created on the first completed game:

| OS | Path |
|---|---|
| Windows | `%LOCALAPPDATA%\Tetonor\statistics.txt` |
| Linux | `~/.local/share/tetonor/statistics.txt` |
| macOS | `~/Library/Application Support/Tetonor/statistics.txt` |

It is a small text file that starts with `TETONOR-STATS 1`. A missing file means empty statistics; an empty, truncated, edited or otherwise damaged file is ignored and replaced by the next completed game (no backup is kept). If the folder cannot be created or written, the game still runs and keeps its statistics for the session, but nothing is saved. Delete the file to reset the statistics.

### Saved game file

The saved game is the file `saved_game.txt` in the same per-user folder as the statistics, written each time you save and read once when the game starts:

| OS | Path |
|---|---|
| Windows | `%LOCALAPPDATA%\Tetonor\saved_game.txt` |
| Linux | `~/.local/share/tetonor/saved_game.txt` |
| macOS | `~/Library/Application Support/Tetonor/saved_game.txt` |

It is a small text file that starts with `TETONOR-SAVE 2` (it records whether a hint was used; files that start with `TETONOR-SAVE 1`, written by earlier versions, still load, as a game without a hint). It holds the puzzle and its solution, so the game can check that a save is solvable before it loads it. A missing file means there is no saved game. An empty, truncated, edited or otherwise damaged file is ignored without any message (no "continue" line appears) and replaced by the next save; there is no checksum, so an edit that keeps the file consistent, such as a shorter time, is accepted. A failed write leaves the previous file untouched. If the folder cannot be created or written, the game still runs and keeps the saved game for the session, but nothing is stored. Delete the file to forget the saved game.

### Configuration file

The chosen theme is saved when you choose it and loaded when the game starts. The file is `config.txt`, in the same per-user folder as `statistics.txt`:

| OS | Path |
|---|---|
| Windows | `%LOCALAPPDATA%\Tetonor\config.txt` |
| Linux | `~/.local/share/tetonor/config.txt` |
| macOS | `~/Library/Application Support/Tetonor/config.txt` |

It is a small text file with two lines, `TETONOR-CONFIG 1` and `theme classic` (or `theme light`). A missing, empty, damaged or unknown file starts the game in the Classic theme, silently; the next theme choice replaces it. If the folder or the file cannot be written, the game still runs and the chosen theme applies for the session, but is not remembered. Delete the file to reset the theme to Classic.

### Difficulty

The level chosen before a game stays fixed for that game, and RESTART starts a new puzzle at the same level. The next game from the main menu asks again, with Medium pre-selected. No level uses the number 0.

| Level | Numbers in the puzzle | Hidden numbers in the available row | Pairs with both numbers hidden |
|---|---|---|---|
| Easy | 1 to 12 | 4 | 0 |
| Medium | 1 to 20 | 8 | 0 |
| Hard | 1 to 29 | 12 | 4 |

On Easy and Medium every hidden number has a visible partner from its own pair, so it can be found from one sum and one product. On Hard some pairs have both numbers hidden. These are structural differences that the tests check; how hard a level feels to a player is not measured. A puzzle may have more than one valid solution, and any valid solution wins. Because Easy uses a smaller range of numbers, unrelated matrix cells can match a sum or product by coincidence more often there than on Hard.
