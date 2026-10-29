#ifndef TETONOR_UI_LAYOUT_H
#define TETONOR_UI_LAYOUT_H

#include <stdbool.h>
#include <stddef.h>

#include "../game/game_port.h"
#include "../game/operand_group.h"

/* Each matrix cell is a block: the matrix box on top and its three operand
 * boxes underneath, sharing one rule. Vertically adjacent blocks share their
 * border rule, so a block row takes LAYOUT_CELL_PITCH_Y lines.
 *
 *   +-------+  row 0   top rule (join rule in block rows 1-3)
 *   |  182  |  row 1   LAYOUT_CELL_VALUE_ROW
 *   +--+-+--+  row 2   LAYOUT_CELL_SPLIT_ROW
 *   |29|x| 7|  row 3   LAYOUT_CELL_GROUP_ROW
 *   +--+-+--+  row 4   next block's join rule (bottom rule in the last row)
 */
#define LAYOUT_ORIGIN_Y 1
#define LAYOUT_ORIGIN_X 2
#define LAYOUT_NUMBER_FIELD_W 2    /* inner width of a group number box */
#define LAYOUT_OPERATION_FIELD_W 1 /* inner width of the operation box */
#define LAYOUT_GROUP_RULES 4       /* vertical lines across a group row */
#define LAYOUT_CELL_W                                                          \
    (LAYOUT_GROUP_RULES + 2 * LAYOUT_NUMBER_FIELD_W + LAYOUT_OPERATION_FIELD_W)
#define LAYOUT_CELL_GAP_X 2   /* blank columns between blocks */
#define LAYOUT_CELL_PITCH_Y 4 /* lines per block row; borders are shared */
#define LAYOUT_CELL_VALUE_ROW 1
#define LAYOUT_CELL_SPLIT_ROW 2
#define LAYOUT_CELL_GROUP_ROW 3
#define LAYOUT_MINI_W 4
#define LAYOUT_MINI_H 3
#define LAYOUT_SECTION_GAP 1 /* the "Available:" label row */
#define LAYOUT_MESSAGE_GAP 1 /* blank rows above the result message */
#define LAYOUT_CORRECT_MESSAGE "CORRECT ANSWER"
#define LAYOUT_WRONG_MESSAGE "WRONG ANSWER"
#define LAYOUT_RESULT_MESSAGE_W ((int)(sizeof LAYOUT_CORRECT_MESSAGE - 1))
#define LAYOUT_UNDO_EMPTY_MESSAGE "Nothing to undo"
/* Drawn at layout_undo_notice_origin(); must not be longer than the undo
 * message, because layout_conflict_notice_origin reserves exactly that width.
 */
#define LAYOUT_REDO_EMPTY_MESSAGE "Nothing to redo"
#define LAYOUT_NOTICE_GAP_X                                                    \
    2 /* blank columns between the result and a notice */
#define LAYOUT_CONFLICT_MARKER '!' /* frames a conflicting group or slot */
/* printf format: count, then LAYOUT_CONFLICT_MARKER. */
#define LAYOUT_CONFLICT_FORMAT "Conflicts: %d (marked %c)"
#define LAYOUT_CONFLICT_NOTICE_SIZE 32 /* bytes, NUL included */
#define LAYOUT_TIMER_LABEL "Time: "
#define LAYOUT_TIMER_GAP_X 4 /* blank columns before the timer */
#define LAYOUT_TIMER_W                                                         \
    ((int)(sizeof LAYOUT_TIMER_LABEL - 1) + TETONOR_ELAPSED_TEXT_SIZE - 1)
/* printf format: the date text ("YYYY-MM-DD"). */
#define LAYOUT_DAILY_BADGE_FORMAT " DAILY CHALLENGE %s "
/* The format without its "%s", plus the date. */
#define LAYOUT_DAILY_BADGE_W                                                   \
    ((int)(sizeof LAYOUT_DAILY_BADGE_FORMAT - 1) - 2 +                         \
     (TETONOR_DATE_TEXT_SIZE - 1))
#define LAYOUT_ADD_SYMBOL '+' /* the operation box shows these */
#define LAYOUT_MULTIPLY_SYMBOL 'x'
/* The side panel right of the matrix: the timer, then the hint, then the
 * uncounted-game notice. */
#define LAYOUT_SIDE_PANEL_GAP_Y 2 /* rows between the timer and the hint */
#define LAYOUT_HINT_LINE_COUNT 2
#define LAYOUT_UNCOUNTED_GAP_Y 1 /* blank rows between hint and notice */
/* printf format: LAYOUT_CONFLICT_MARKER. */
#define LAYOUT_CONFLICT_HINT_FORMAT "No hint: fix the boxes marked %c"
#define LAYOUT_UNCOUNTED_LINE_COUNT 2
#define LAYOUT_HINT_TEXT_SIZE 40 /* bytes of one hint line, NUL included */
#define LAYOUT_HINT_MARKER '?'   /* frames the box a hint suggests */
#define LAYOUT_CONFIRM_LABEL "CONFIRM"
#define LAYOUT_RESTART_LABEL "RESTART"
#define LAYOUT_BUTTON_PADDING 1 /* blank columns each side of the label */
#define LAYOUT_BUTTON_H 3
#define LAYOUT_BUTTON_W                                                        \
    ((int)(sizeof LAYOUT_CONFIRM_LABEL - 1) + 2 * (LAYOUT_BUTTON_PADDING + 1))
#define LAYOUT_BUTTON_GAP_X                                                    \
    2 /* columns between last available box and button */
#define LAYOUT_MIN_ROWS 27
#define LAYOUT_MIN_COLS 80
/* printf format: columns, then rows. */
#define LAYOUT_ENLARGE_FORMAT "Please enlarge the terminal to at least %dx%d"
#define LAYOUT_MENU_H 9
#define LAYOUT_MENU_W 50
#define LAYOUT_MENU_TITLE_ROW 1
#define LAYOUT_MENU_CONTINUE_ROW 2 /* the free row under the title */
#define LAYOUT_MENU_START_ROW 3
#define LAYOUT_MENU_DAILY_ROW 4
#define LAYOUT_MENU_STATS_ROW 5
#define LAYOUT_MENU_THEME_ROW 6
#define LAYOUT_MENU_EXIT_ROW 7
#define LAYOUT_MENU_TEXT_COL 2
#define LAYOUT_MENU_CONTINUE_HINT "Press C to continue the saved game"
#define LAYOUT_DIFFICULTY_MENU_H 10
#define LAYOUT_DIFFICULTY_MENU_W LAYOUT_MENU_W
#define LAYOUT_DIFFICULTY_TITLE_ROW 1
#define LAYOUT_DIFFICULTY_FIRST_OPTION_ROW 3
#define LAYOUT_DIFFICULTY_CHOOSE_ROW 7
#define LAYOUT_DIFFICULTY_BACK_ROW 8
#define LAYOUT_HELP_LINE_COUNT 3
#define LAYOUT_PAUSE_MENU_H 9
#define LAYOUT_PAUSE_MENU_W LAYOUT_MENU_W
#define LAYOUT_PAUSE_TITLE_ROW 1
#define LAYOUT_PAUSE_NOTICE_ROW 2 /* the save notice, under the title */
#define LAYOUT_PAUSE_FIRST_OPTION_ROW 3
#define LAYOUT_PAUSE_SAVE_ROW 5
#define LAYOUT_PAUSE_CHOOSE_ROW 6
#define LAYOUT_PAUSE_BACK_ROW 7
#define LAYOUT_PAUSE_TITLE "Paused"
#define LAYOUT_PAUSE_CHOOSE_HINT "UP/DOWN to choose, SPACE or ENTER to select"
#define LAYOUT_PAUSE_BACK_HINT "Press ESC or P to resume, T for theme"
#define LAYOUT_PAUSE_SAVE_HINT "Press S to save the game"
#define LAYOUT_PAUSE_NO_SAVE_HINT "Daily challenges are not saved"
#define LAYOUT_THEME_MENU_W LAYOUT_MENU_W
#define LAYOUT_THEME_TITLE_ROW 1
#define LAYOUT_THEME_FIRST_OPTION_ROW 3
#define LAYOUT_THEME_CHOOSE_ROW                                                \
    (LAYOUT_THEME_FIRST_OPTION_ROW + PALETTE_THEME_COUNT + 1)
#define LAYOUT_THEME_BACK_ROW (LAYOUT_THEME_CHOOSE_ROW + 1)
#define LAYOUT_THEME_MENU_H (LAYOUT_THEME_BACK_ROW + 2)
#define LAYOUT_THEME_TITLE "Choose theme"
#define LAYOUT_THEME_CHOOSE_HINT LAYOUT_PAUSE_CHOOSE_HINT
#define LAYOUT_THEME_BACK_HINT "Press ESC or T to go back"
#define LAYOUT_PROMPT_H 10
#define LAYOUT_PROMPT_W LAYOUT_MENU_W
#define LAYOUT_PROMPT_TITLE_ROW 1
#define LAYOUT_PROMPT_WARNING_ROW 2
#define LAYOUT_PROMPT_FIRST_OPTION_ROW 4
#define LAYOUT_PROMPT_CHOOSE_ROW 7
#define LAYOUT_PROMPT_BACK_ROW 8
#define LAYOUT_PROMPT_WARNING "Progress in this game will be lost."
#define LAYOUT_PROMPT_CHOOSE_HINT "Y or N, or UP/DOWN and SPACE or ENTER"
#define LAYOUT_PROMPT_BACK_HINT "Press ESC to cancel"
#define LAYOUT_VICTORY_H 13
#define LAYOUT_VICTORY_W LAYOUT_MENU_W
#define LAYOUT_VICTORY_TITLE_ROW 1
#define LAYOUT_VICTORY_FIRST_SUMMARY_ROW 3
#define LAYOUT_VICTORY_FIRST_OPTION_ROW 7
#define LAYOUT_VICTORY_CHOOSE_ROW 10
#define LAYOUT_VICTORY_SHORTCUT_ROW 11
#define LAYOUT_VICTORY_TITLE "Puzzle solved!"
#define LAYOUT_VICTORY_CHOOSE_HINT LAYOUT_PAUSE_CHOOSE_HINT
#define LAYOUT_VICTORY_SHORTCUT_HINT "N new game   M or ESC main menu"
#define LAYOUT_STATS_MENU_H 10
#define LAYOUT_STATS_MENU_W LAYOUT_MENU_W
#define LAYOUT_STATS_TITLE_ROW 1
#define LAYOUT_STATS_HEADER_ROW 3
#define LAYOUT_STATS_FIRST_ROW 4
#define LAYOUT_STATS_BACK_ROW 8
#define LAYOUT_STATS_COUNT_COL 14
#define LAYOUT_STATS_COUNT_W 10 /* INT32_MAX has ten digits */
#define LAYOUT_STATS_BEST_COL 27
#define LAYOUT_STATS_AVERAGE_COL 37

typedef struct
{
    int y;
    int x;
} LayoutPoint;

/* (container - content) / 2, clamped to >= 0. */
int layout_centered_offset(int container_size, int content_size);

LayoutPoint layout_centered_window_origin(int screen_rows, int screen_cols,
                                          int height, int width);

/* Window row of a level's option line on the difficulty screen. */
int layout_difficulty_option_row(int difficulty);

/* Top-left corner of the block of the matrix cell at (row, col). */
LayoutPoint layout_sum_cell_origin(int row, int col);

/* Row of the bottom rule of the last block row. */
int layout_grid_bottom_row(void);

/* First inner column, on the group row, of one box of the cell's group. */
LayoutPoint layout_group_field_origin(int row, int col, GroupField field);

/* Inner width of one box of a group. */
int layout_group_field_width(GroupField field);

/* Row and column of the "Available:" label, above the available boxes. */
LayoutPoint layout_available_label_origin(void);
LayoutPoint layout_available_cell_origin(int index);

/* Top-left corner of the CONFIRM button, beside the available row. */
LayoutPoint layout_confirm_button_origin(void);

/* Top-left corner of the RESTART button, directly below CONFIRM. */
LayoutPoint layout_restart_button_origin(void);

/* Row/column of the result message, below the RESTART button. */
LayoutPoint layout_result_message_origin(void);

/* Row/column of the nothing-to-undo or nothing-to-redo notice: on the result
 * row, right of the longer result text. */
LayoutPoint layout_undo_notice_origin(void);

/* Row/column of the conflict notice: on the result row, right of the
 * nothing-to-undo notice. */
LayoutPoint layout_conflict_notice_origin(void);

/* Writes the conflict notice for count, clamped to
 * 0..TETONOR_CONFLICT_COUNT_MAX, into text (size bytes, truncated, always
 * NUL-terminated when size > 0). */
void layout_format_conflict_notice(int count, char* text, size_t size);

/* Top-left corner of the timer text, right of the first grid row. */
LayoutPoint layout_timer_origin(void);

/* Top-left of the daily challenge badge: row 0, above the board and every
 * centred panel. */
LayoutPoint layout_daily_badge_origin(void);

/* Row/column of hint line (0..LAYOUT_HINT_LINE_COUNT-1): in the timer's column,
 * LAYOUT_SIDE_PANEL_GAP_Y rows below the timer. */
LayoutPoint layout_hint_line_origin(int line);

/* Row/column of uncounted-notice line (0..LAYOUT_UNCOUNTED_LINE_COUNT-1):
 * below the hint lines, after one blank row. */
LayoutPoint layout_uncounted_line_origin(int line);

/* Columns of the side panel: from the timer's column to the right edge of the
 * minimum terminal. */
int layout_side_panel_width(void);

/* Writes line (0..LAYOUT_HINT_LINE_COUNT-1) of the text that tells the player
 * what hint is: the suggestion (1-based box, row and column numbers) or why
 * there is none. "" for any other line. Truncated, and NUL-terminated when
 * size > 0; size 0 writes nothing. Every line fits layout_side_panel_width().
 */
void layout_format_hint_line(const BoardHint* hint, int line, char* text,
                             size_t size);

/* One line of the notice shown once a hint was used (index
 * 0..LAYOUT_UNCOUNTED_LINE_COUNT-1); "" out of range. */
const char* layout_uncounted_line(int index);

/* One line of the controls help (index 0..LAYOUT_HELP_LINE_COUNT-1); "" out of
 * range. */
const char* layout_help_line(int index);

/* Row/column of help line index: left of RESTART, one line per row from its
 * top. */
LayoutPoint layout_help_line_origin(int index);

/* Columns available to a help line: from LAYOUT_ORIGIN_X to the button gap. */
int layout_help_width(void);

/* Window row of a theme's option line on the theme screen. */
int layout_theme_option_row(int theme);

/* Window row of a pause-menu option / a prompt option. */
int layout_pause_option_row(int option);
int layout_prompt_option_row(int option);

/* Window row of a summary line / an option on the victory screen. */
int layout_victory_summary_row(int line);
int layout_victory_option_row(int option);

/* Window row of a level's line on the statistics screen. */
int layout_stats_row(int difficulty);

/* True when a terminal of rows x cols holds the whole board. */
bool layout_fits(int rows, int cols);

#endif
