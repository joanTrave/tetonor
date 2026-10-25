#ifndef TETONOR_UI_GAME_ACTION_H
#define TETONOR_UI_GAME_ACTION_H

/* What the caller must do now. */
typedef enum
{
    GAME_ACTION_NONE,
    GAME_ACTION_NEW_GAME,
    GAME_ACTION_MAIN_MENU /* abandon the game */
} GameAction;

#endif
