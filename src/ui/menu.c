#include <curses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"
#include "difficulty_menu.h"
#include "keys.h"
#include "layout.h"
#include "stats_menu.h"
#include "ui.h"
#include "../game/game_port.h"

static const char menu_title[]       = "Welcome to Tetonor";
static const char menu_start_hint[]  = "Press SPACE or ENTER to start game";
static const char menu_stats_hint[]  = "Press S to view statistics";
static const char menu_exit_hint[]   = "Press ESC to exit";

static const char difficulty_title[] = "Choose difficulty";
static const char difficulty_choose_hint[] =
    "UP/DOWN to choose, SPACE or ENTER to start";
static const char back_hint[]            = "Press ESC to go back";

static const char stats_title[]          = "Statistics";
static const char stats_label_header[]   = "Difficulty";
static const char stats_count_header[]   = "Completed";
static const char stats_best_header[]    = "Best";
static const char stats_average_header[] = "Average";

static void clear_screen(void);
static WINDOW* create_centered_window(int height, int width);
static void draw_main_menu(WINDOW* win);
static MenuResult read_menu_choice(WINDOW* win);
static bool run_main_menu(MenuResult* result);
static MenuKeyCodes menu_key_codes(void);
static void draw_difficulty_option(WINDOW* win, Difficulty level,
                                   Difficulty selected);
static void draw_difficulty_menu(WINDOW* win, const DifficultyMenu* menu);
static DifficultyMenuCommand read_difficulty_command(WINDOW* win);
static bool run_difficulty_menu(DifficultyMenu* menu);
static void draw_stats_row(WINDOW* win, const Statistics* statistics,
                           int difficulty);
static void draw_stats_menu(WINDOW* win, const Statistics* statistics);
static StatsMenuCommand read_stats_command(WINDOW* win);
static bool run_stats_menu(const Statistics* statistics);
static int run_game_session(Difficulty difficulty, StatsStore* stats_store);

int ui_show_start_menu(StatsStore* stats_store)
{
    for (;;)
    {
        clear_screen();
        MenuResult menu_result;
        if (!run_main_menu(&menu_result))
        {
            return EXIT_FAILURE;
        }
        if (menu_result == MENU_STATISTICS)
        {
            clear_screen();
            if (!run_stats_menu(&stats_store->statistics))
            {
                return EXIT_FAILURE;
            }
            continue;
        }
        if (menu_result == MENU_EXIT)
        {
            return EXIT_SUCCESS;
        }

        clear_screen();
        DifficultyMenu difficulty_menu;
        if (!run_difficulty_menu(&difficulty_menu))
        {
            return EXIT_FAILURE;
        }
        if (difficulty_menu.status == DIFFICULTY_MENU_CANCELLED)
        {
            continue;
        }

        clear_screen();
        int status = run_game_session(difficulty_menu.selected, stats_store);
        if (status != EXIT_SUCCESS)
        {
            return status;
        }
    }
}

static void clear_screen(void)
{
    erase();
    refresh();
}

static WINDOW* create_centered_window(int height, int width)
{
    int rows;
    int cols;
    getmaxyx(stdscr, rows, cols);

    LayoutPoint origin =
        layout_centered_window_origin(rows, cols, height, width);
    WINDOW* win = newwin(height, width, origin.y, origin.x);
    if (win != NULL)
    {
        keypad(win, TRUE);
    }
    return win;
}

static void draw_main_menu(WINDOW* win)
{
    box(win, 0, 0);

    int title_col =
        layout_centered_offset(LAYOUT_MENU_W, (int)strlen(menu_title));
    mvwprintw(win, LAYOUT_MENU_TITLE_ROW, title_col, "%s", menu_title);
    mvwprintw(win, LAYOUT_MENU_START_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              menu_start_hint);
    mvwprintw(win, LAYOUT_MENU_STATS_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              menu_stats_hint);
    mvwprintw(win, LAYOUT_MENU_EXIT_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              menu_exit_hint);

    wrefresh(win);
}

static MenuResult read_menu_choice(WINDOW* win)
{
    MenuResult result = MENU_NONE;
    while (result == MENU_NONE)
    {
        int key = wgetch(win);
        if (key == ERR)
        {
            return MENU_EXIT; /* input closed: leave instead of spinning */
        }
        result = menu_result_from_key(key, KEY_ENTER);
    }
    return result;
}

/* Returns false when the window cannot be created. */
static bool run_main_menu(MenuResult* result)
{
    WINDOW* win = create_centered_window(LAYOUT_MENU_H, LAYOUT_MENU_W);
    if (win == NULL)
    {
        return false;
    }
    draw_main_menu(win);
    *result = read_menu_choice(win);
    delwin(win);
    return true;
}

static MenuKeyCodes menu_key_codes(void)
{
    MenuKeyCodes key_codes;
    key_codes.enter = KEY_ENTER;
    key_codes.up    = KEY_UP;
    key_codes.down  = KEY_DOWN;
    return key_codes;
}

/* The selected option is marked with "> " and reversed video, so the cue does
 * not depend on colour. */
static void draw_difficulty_option(WINDOW* win, Difficulty level,
                                   Difficulty selected)
{
    bool is_selected = level == selected;
    int row          = layout_difficulty_option_row(level);
    if (is_selected)
    {
        wattron(win, A_REVERSE);
    }
    mvwprintw(win, row, LAYOUT_MENU_TEXT_COL, "%s%s", is_selected ? "> " : "  ",
              difficulty_menu_label(level));
    if (is_selected)
    {
        wattroff(win, A_REVERSE);
    }
}

static void draw_difficulty_menu(WINDOW* win, const DifficultyMenu* menu)
{
    werase(win);
    box(win, 0, 0);

    int title_col = layout_centered_offset(LAYOUT_DIFFICULTY_MENU_W,
                                           (int)strlen(difficulty_title));
    mvwprintw(win, LAYOUT_DIFFICULTY_TITLE_ROW, title_col, "%s",
              difficulty_title);
    for (int level = 0; level < DIFFICULTY_COUNT; level++)
    {
        draw_difficulty_option(win, (Difficulty)level, menu->selected);
    }
    mvwprintw(win, LAYOUT_DIFFICULTY_CHOOSE_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              difficulty_choose_hint);
    mvwprintw(win, LAYOUT_DIFFICULTY_BACK_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              back_hint);

    wrefresh(win);
}

static DifficultyMenuCommand read_difficulty_command(WINDOW* win)
{
    int key = wgetch(win);
    if (key == ERR)
    {
        return DIFFICULTY_COMMAND_BACK; /* input closed: leave, do not spin */
    }
    MenuKeyCodes key_codes = menu_key_codes();
    return difficulty_menu_command_from_key(key, &key_codes);
}

/* Returns false when the window cannot be created. */
static bool run_difficulty_menu(DifficultyMenu* menu)
{
    WINDOW* win = create_centered_window(LAYOUT_DIFFICULTY_MENU_H,
                                         LAYOUT_DIFFICULTY_MENU_W);
    if (win == NULL)
    {
        return false;
    }
    init_difficulty_menu(menu);
    while (menu->status == DIFFICULTY_MENU_OPEN)
    {
        draw_difficulty_menu(win, menu);
        apply_difficulty_menu_command(menu, read_difficulty_command(win));
    }
    delwin(win);
    return true;
}

static void draw_stats_row(WINDOW* win, const Statistics* statistics,
                           int difficulty)
{
    StatsMenuRow row;
    build_stats_menu_row(statistics, difficulty, &row);
    int y = layout_stats_row(difficulty);
    mvwprintw(win, y, LAYOUT_MENU_TEXT_COL, "%s", row.label);
    mvwprintw(win, y, LAYOUT_STATS_COUNT_COL, "%*d", LAYOUT_STATS_COUNT_W,
              (int)row.completed_count);
    mvwprintw(win, y, LAYOUT_STATS_BEST_COL, "%s", row.best);
    mvwprintw(win, y, LAYOUT_STATS_AVERAGE_COL, "%s", row.average);
}

static void draw_stats_menu(WINDOW* win, const Statistics* statistics)
{
    werase(win);
    box(win, 0, 0);

    int title_col =
        layout_centered_offset(LAYOUT_STATS_MENU_W, (int)strlen(stats_title));
    mvwprintw(win, LAYOUT_STATS_TITLE_ROW, title_col, "%s", stats_title);
    mvwprintw(win, LAYOUT_STATS_HEADER_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              stats_label_header);
    mvwprintw(win, LAYOUT_STATS_HEADER_ROW, LAYOUT_STATS_COUNT_COL, "%*s",
              LAYOUT_STATS_COUNT_W, stats_count_header);
    mvwprintw(win, LAYOUT_STATS_HEADER_ROW, LAYOUT_STATS_BEST_COL, "%s",
              stats_best_header);
    mvwprintw(win, LAYOUT_STATS_HEADER_ROW, LAYOUT_STATS_AVERAGE_COL, "%s",
              stats_average_header);
    for (int level = 0; level < DIFFICULTY_COUNT; level++)
    {
        draw_stats_row(win, statistics, level);
    }
    mvwprintw(win, LAYOUT_STATS_BACK_ROW, LAYOUT_MENU_TEXT_COL, "%s",
              back_hint);

    wrefresh(win);
}

static StatsMenuCommand read_stats_command(WINDOW* win)
{
    int key = wgetch(win);
    if (key == ERR)
    {
        return STATS_MENU_COMMAND_BACK; /* input closed: leave, do not spin */
    }
    return stats_menu_command_from_key(key);
}

/* Returns false when the window cannot be created. */
static bool run_stats_menu(const Statistics* statistics)
{
    WINDOW* win =
        create_centered_window(LAYOUT_STATS_MENU_H, LAYOUT_STATS_MENU_W);
    if (win == NULL)
    {
        return false;
    }
    draw_stats_menu(win, statistics);
    StatsMenuCommand command;
    do
    {
        command = read_stats_command(win);
    } while (command != STATS_MENU_COMMAND_BACK);
    delwin(win);
    return true;
}

static int run_game_session(Difficulty difficulty, StatsStore* stats_store)
{
    GamePort* game_port = create_game_port();
    if (game_port == NULL)
    {
        return EXIT_FAILURE;
    }
    game_port->difficulty = difficulty;
    int status            = ui_run(game_port, stats_store);
    destroy_game_port(game_port);
    return status;
}
