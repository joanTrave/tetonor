# tetonor - Best Practices

This file is read by reviewers (human, PR-Agent and the `reviewer` agent) to enforce the project's
coding standards. Violations are flagged as "Organization best practice" suggestions.

Tetonor is a terminal game written in **portable C11** with **curses**. It must build with
`-std=c11 -Wall -Wextra -Werror -pedantic` and run unchanged on Linux, macOS and Windows. The
detailed rules live in `.claude/skills/c/`, `.claude/skills/tui/`, `.claude/skills/architecture/`,
`.claude/skills/naming/` and `.claude/skills/testing/`; this file lists the patterns reviewers
check most often.

---

## Pattern 1: Portable Game Core

Code in `src/game/` uses only the C11 standard library. It never includes curses, POSIX or Windows
headers, and never does terminal I/O. This keeps the rules portable and unit-testable.

Example code before:

```c
#include <ncurses.h>
#include <unistd.h>

#include "number_generation.h"

void generate_and_show(int difficulty)
{
    NumberGeneration* ng = generate_random_numbers(difficulty);
    mvprintw(0, 0, "%d", ng->available_array[0]);
    usleep(1000);
}
```

Example code after:

```c
#include <stdbool.h>
#include <stdlib.h>

#include "number_generation.h"

bool number_generation_is_hidden(const NumberGeneration* ng, size_t index)
{
    return ng->available_array[index] == TETONOR_HIDDEN_NUMBER;
}
```

---

## Pattern 2: UI Talks to the Game Only Through the Port

`src/ui/` reaches the game exclusively through `GamePort` (`src/game/game_port.h`). The UI never
re-implements a game rule and never includes other `game/` internals. New capabilities are added
to the core and exposed as a port function.

Example code before:

```c
#include "../game/number_generation.h"

static bool is_solved(NumberGeneration* ng)
{
    /* game rule re-implemented inside the UI */
    for (int i = 0; i < 16; i++)
        if (ng->available_array[i] == -1)
            return false;
    return true;
}
```

Example code after:

```c
#include "../game/game_port.h"

static void draw_status(const GamePort* game_port, const NumberGeneration* ng)
{
    if (game_port->is_solved_f(ng))
    {
        mvprintw(0, 0, "Solved!");
    }
}
```

---

## Pattern 3: Portable Curses Usage

Include `<curses.h>` (shipped by ncurses and PDCurses alike), not `<ncurses.h>`. Use only the
X/Open curses API; guard ncurses-only extensions. Link the curses library through a Makefile
variable so Windows can use PDCurses.

Example code before:

```c
#include <ncurses.h>

void init_screen(void)
{
    initscr();
    set_escdelay(25);
}
```

```make
LDFLAGS=-lncurses
```

Example code after:

```c
#include <curses.h>

void init_screen(void)
{
    initscr();
#ifdef NCURSES_VERSION
    set_escdelay(25);
#endif
}
```

```make
CURSES_LIB ?= -lncurses   # make CURSES_LIB=-lpdcurses on Windows
LDFLAGS    = $(CURSES_LIB)
```

---

## Pattern 4: Checked Allocations With Paired Release

Every `malloc`/`calloc` result is checked. On failure, release what was already allocated and
return `NULL`. Every `create_*`/`generate_*` has a matching `destroy_*`/`free_*` in the same module
that accepts `NULL`.

Example code before:

```c
GamePort* create_game_port(void)
{
    GamePort* game_port   = malloc(sizeof(GamePort));
    game_port->difficulty = 1;
    return game_port;
}
```

Example code after:

```c
GamePort* create_game_port(void)
{
    GamePort* game_port = malloc(sizeof *game_port);
    if (game_port == NULL)
    {
        return NULL;
    }
    game_port->difficulty = TETONOR_DEFAULT_DIFFICULTY;
    return game_port;
}

void destroy_game_port(GamePort* game_port)
{
    free(game_port); /* free(NULL) is a no-op */
}
```

---

## Pattern 5: Fixed-Size Data in Fixed-Size Arrays

The board and the available numbers have fixed sizes. Store them as arrays inside the struct
instead of nested heap allocations: one allocation, one `free`, no partial-failure paths.

Example code before:

```c
typedef struct
{
    int** sum_matrix;
    int* available_array;
} NumberGeneration;
```

Example code after:

```c
#define TETONOR_GRID_SIZE 4
#define TETONOR_CELL_COUNT (TETONOR_GRID_SIZE * TETONOR_GRID_SIZE)

typedef struct
{
    int sum_matrix[TETONOR_GRID_SIZE][TETONOR_GRID_SIZE];
    int available_array[TETONOR_CELL_COUNT];
} NumberGeneration;
```

---

## Pattern 6: `static` for Private Functions, No Leading Underscores

Helpers that are not part of a module's header are `static`. Identifiers starting with `_` at file
scope are reserved by the C standard (C11 §7.1.3) and must not be introduced.

Example code before:

```c
void _swap(int* a, int* b);
int* _get_randomized_array_positions(void);
```

Example code after:

```c
static void swap_ints(int* a, int* b);
static void shuffle_positions(int positions[], size_t count);
```

---

## Pattern 7: Named Constants, No Magic Numbers

Dimensions, value ranges, sentinels and key codes are defined once, in the header of the module
that owns the concept, and reused everywhere.

Example code before:

```c
number_generation->available_array[random_position] = -1;   /* game/ */
if (ng->available_array[i] != -1)                           /* ui/   */
int value_1 = rand() % 30;
if (ch == 27) // ESC
```

Example code after:

```c
/* number_generation.h */
#define TETONOR_HIDDEN_NUMBER (-1)
#define TETONOR_MAX_VALUE 29

/* ui/keys.h */
enum
{
    TETONOR_KEY_ESCAPE = 27
};

number_generation->available_array[random_position] = TETONOR_HIDDEN_NUMBER;
if (ng->available_array[i] != TETONOR_HIDDEN_NUMBER)
int value_1 = rand() % (TETONOR_MAX_VALUE + 1);
if (ch == TETONOR_KEY_ESCAPE)
```

---

## Pattern 8: Curses Lifecycle and Resource Release on Every Exit Path

`initscr()` is called once per process and `endwin()` once before exit, on every path. Screens do
not re-initialise curses. Every resource a function creates (windows, ports, game state) is
released on every return path. Nothing is printed to `stdout`/`stderr` while curses is active.

Example code before:

```c
void ui_show_start_menu(void)
{
    init_screen();
    WINDOW* win         = _init_window();
    GamePort* game_port = create_game_port();
    _draw_main_menu(win);
    if (_select_menu_option(win) == MENU_EXIT)
    {
        endwin();
        return; /* game_port leaks */
    }
    ui_run(game_port); /* calls initscr() a second time */
    destroy_game_port(game_port);
}
```

Example code after:

```c
/* main.c owns the terminal lifecycle */
int main(void)
{
    srand((unsigned)time(NULL));
    init_screen();
    int status = ui_show_start_menu();
    endwin();
    return status;
}

/* menu.c */
int ui_show_start_menu(void)
{
    if (show_main_menu() == MENU_EXIT)
    {
        return EXIT_SUCCESS;
    }
    GamePort* game_port = create_game_port();
    if (game_port == NULL)
    {
        return EXIT_FAILURE;
    }
    ui_run(game_port);
    destroy_game_port(game_port);
    return EXIT_SUCCESS;
}
```

---

## Pattern 9: Keys Are `int`, Compared Against Named Constants

`getch()`/`wgetch()` return `int`; never store them in `char`. Use curses `KEY_*` constants and
project constants. Accept every Enter variant. Each screen maps keys to a result `enum` in one loop.

Example code before:

```c
char ch = getch();
if (ch == 10)
    start_game();
```

Example code after:

```c
static MenuResult read_menu_choice(WINDOW* win)
{
    for (;;)
    {
        int ch = wgetch(win);
        switch (ch)
        {
            case ' ':
            case '\n':
            case '\r':
            case KEY_ENTER:
                return MENU_START;
            case TETONOR_KEY_ESCAPE:
                return MENU_EXIT;
            default:
                break; /* ignore unknown keys */
        }
    }
}
```

---

## Pattern 10: Terminal Size and Resize Handling

Check the terminal size before drawing and handle `KEY_RESIZE`. If the layout does not fit, show a
short message instead of drawing off-screen. Layout maths lives in pure functions so it can be
tested without a terminal.

Example code before:

```c
void print_layout(GamePort* game_port)
{
    /* draws at fixed coordinates regardless of terminal size */
    _print_sum_matrix(ng);
    _print_available_array(ng);
}
```

Example code after:

```c
/* layout.c — pure, unit-testable */
bool layout_fits(int rows, int cols)
{
    return rows >= LAYOUT_MIN_ROWS && cols >= LAYOUT_MIN_COLS;
}

/* print_layout.c */
void print_layout(const GamePort* game_port, const NumberGeneration* ng)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    erase();
    if (!layout_fits(rows, cols))
    {
        mvprintw(0, 0, "Please enlarge the terminal (min %dx%d)",
                 LAYOUT_MIN_COLS, LAYOUT_MIN_ROWS);
    }
    else
    {
        print_sum_matrix(ng);
        print_available_array(ng);
    }
    refresh();
}
```

---

## Pattern 11: Deterministic, Injectable Randomness

Seed only in `main`. Game-core functions that need randomness receive a random source so tests
can make them deterministic. Shuffles use the unbiased Fisher–Yates form.

Example code before:

```c
for (int i = 0; i < 16; i++)
{
    size_t j = rand() % (16 - i); /* biased: j is not in [i, 16) */
    _swap(&positions[i], &positions[j]);
}
```

Example code after:

```c
typedef int (*RandomFn)(void);

static void shuffle_positions(int positions[], size_t count, RandomFn rng)
{
    for (size_t i = 0; i + 1 < count; i++)
    {
        size_t j = i + (size_t)rng() % (count - i);
        swap_ints(&positions[i], &positions[j]);
    }
}
```

---

## Pattern 12: Deterministic, Invariant-Based Unit Tests

Tests live under `tests/` mirroring `src/`, never link curses, use a fixed seed or stub random
source, and assert invariants of the generated puzzle rather than specific random values. Every
allocation in a test is released.

Example code before:

```c
void test_generation(void)
{
    srand(time(NULL));
    NumberGeneration* ng = generate_random_numbers(1);
    ASSERT_EQ_INT(ng->sum_matrix[0][0], 12);
}
```

Example code after:

```c
static void test_generate_random_numbers_hides_difficulty_plus_four_numbers(void)
{
    srand(42);
    NumberGeneration* ng = generate_random_numbers(1);
    ASSERT_NOT_NULL(ng);

    int hidden = 0;
    for (size_t i = 0; i < TETONOR_CELL_COUNT; i++)
    {
        if (ng->available_array[i] == TETONOR_HIDDEN_NUMBER)
        {
            hidden++;
        }
    }
    ASSERT_EQ_INT(hidden, 1 + 4);

    free_number_generation(ng);
}
```

---

## Pattern 13: Naming Conventions

| Type | Convention | Example |
|------|-----------|---------|
| Struct / enum typedefs | PascalCase | `NumberGeneration`, `MenuResult` |
| Enum constants | UPPER_SNAKE_CASE with concept prefix | `MENU_START`, `MENU_EXIT` |
| Public functions | snake_case, verb first; UI entry points `ui_` | `generate_random_numbers`, `ui_run` |
| Private functions | `static` snake_case, no leading `_` | `static void draw_box(...)` |
| Port function pointers | function name + `_f` | `free_number_generation_f` |
| Macros / constants | UPPER_SNAKE_CASE, project-wide ones `TETONOR_` | `TETONOR_GRID_SIZE` |
| Include guards | `TETONOR_<NAME>_H` | `TETONOR_GAME_PORT_H` |
| Lifecycle pairs | `create_`/`destroy_`, `generate_`/`free_` | `create_game_port` / `destroy_game_port` |
| Files | snake_case `.c`/`.h` pairs | `number_generation.c` |
| Test files / functions | `test_<stem>.c` / `test_<unit>_<scenario>` | `test_number_generation.c` |
