#ifndef TETONOR_UI_MENU_H
#define TETONOR_UI_MENU_H

typedef enum
{
    MENU_START,
    MENU_EXIT
} MenuResult;

MenuResult ui_show_start_menu(void);

#endif
