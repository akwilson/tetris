/**
 * Graphics drawing functions
 */
#pragma once

typedef enum color
{
    BLACK,
    YELLOW,
    GREEN,
    PINK,
    BLUE,
    RED,
    DARK
} color;

/**
 * Struct to hold graphics data
 */
typedef struct graphics graphics;

/*
 * Starts up SDL and creates window
 */
graphics *init_graphics(void);

/**
 * Clears the screen ready for the next round of updates
 */
void clear_frame(graphics *graphics);

/*
 * Render a text message
 */
void render_message(graphics *graphics, char* message, int x, int y);

/**
 * Render a rectangle
 */
void render_quad(graphics *graphics, int x, int y, int width, int height, int filled, color color);

/**
 * Update the screen
 */
void commit_to_screen(graphics *graphics);

/*
 * Frees media and shuts down SDL
 */
void close_graphics(graphics *graphics);
