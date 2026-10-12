#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ui/menu.h"
#include "ui/screen.h"

static void init_random_generator(void)
{
    srand((unsigned)time(NULL));
}

int main(void)
{
    init_random_generator();
    init_screen();
    int status = ui_show_start_menu();
    end_screen();
    if (status != EXIT_SUCCESS)
    {
        fprintf(stderr, "tetonor: failed to start the game\n");
    }
    return status;
}
