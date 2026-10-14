#ifndef TETONOR_PALETTE_H
#define TETONOR_PALETTE_H

/* Abstract colours. The UI translates them to terminal colours. The first
 * eight are the basic colours every colour terminal has. */
typedef enum
{
    PALETTE_COLOR_BLACK,
    PALETTE_COLOR_RED,
    PALETTE_COLOR_GREEN,
    PALETTE_COLOR_YELLOW,
    PALETTE_COLOR_BLUE,
    PALETTE_COLOR_MAGENTA,
    PALETTE_COLOR_CYAN,
    PALETTE_COLOR_WHITE,
    PALETTE_COLOR_DARK_GREY,   /* needs 16 colours */
    PALETTE_COLOR_BRIGHT_BLUE, /* needs 16 colours */
    PALETTE_COLOR_BRIGHT_GREEN /* needs 16 colours */
} PaletteColor;

/* What a colour is used for. */
typedef enum
{
    PALETTE_ROLE_ENTERED,          /* player-written numbers */
    PALETTE_ROLE_CORRECT,          /* CORRECT ANSWER */
    PALETTE_ROLE_WRONG,            /* WRONG ANSWER */
    PALETTE_ROLE_USED_ONCE,        /* given strip number holding one use */
    PALETTE_ROLE_USED_TWICE,       /* given strip number holding both uses */
    PALETTE_ROLE_USED_ONCE_TYPED,  /* typed strip number holding one use */
    PALETTE_ROLE_USED_TWICE_TYPED, /* typed strip number holding both uses */
    PALETTE_ROLE_COUNT
} PaletteRole;

typedef struct
{
    PaletteColor color;    /* preferred colour */
    PaletteColor fallback; /* basic colour when color cannot be shown */
} PaletteEntry;

/* Every colour of the game. One instance lives in Board. A plain value with no
 * pointers, so a whole palette can be copied in. */
typedef struct
{
    PaletteEntry foregrounds[PALETTE_ROLE_COUNT];
    PaletteEntry background;
} Palette;

/* Fills palette with the default colours. Acquires nothing. */
void init_palette(Palette* palette);

#endif
