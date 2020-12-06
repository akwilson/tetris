#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "tetronimoes.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define CELL_SIZE 25
#define GRID_X_OFFSET 25
#define GRID_Y_OFFSET 25
#define GRID_CELL_WIDTH 12
#define GRID_CELL_HEIGHT 18
#define GRID_WIDTH GRID_CELL_WIDTH * CELL_SIZE
#define GRID_HEIGHT GRID_CELL_HEIGHT * CELL_SIZE
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME (1000 / SCREEN_FPS)

// The window we'll be rendering to
static SDL_Window *window;

// The renderer to draw the texture on the window
static SDL_Renderer *renderer;

typedef enum color
{
    PURPLE = 1,
    GREEN,
    BLUE,
    YELLOW,
    RED
} color;

/**
 * An in-play tetronimo
 */
typedef struct shape
{
    tetronimo *tetronimo; // the selected tetronimo
    color color;          // index to the color array
    int x;                // x pixel position relative to the top left of the grid
    int y;                // y pixel position relative to the top left of the grid
} shape;

// Macros to convert pixel positions to positions in the grid array
#define CONVERT_TO_X_GRID(x) (x - GRID_X_OFFSET) / CELL_SIZE
#define CONVERT_TO_Y_GRID(y) (y - GRID_Y_OFFSET) / CELL_SIZE

// == SDL STUFF ======================================

/*
 * Starts up SDL and creates window
 */
static int init()
{
    // Initialise SDL and the SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL init failed. SDL_Error:%s\n", SDL_GetError());
        return 1;
    }

    // Create window
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        fprintf(stderr, "Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "Renderer could not be created. SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    return 0;
}

/*
 * Frees media and shuts down SDL
 */
static void close()
{
    // Destroy window and renderer
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = 0;
    window = 0;

    // Quit SDL subsys
    SDL_Quit();
}

static void set_render_color(color color)
{
    switch (color)
    {
    case PURPLE:
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0xFF, 0xFF);
        break;
    case GREEN:
        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
        break;
    case BLUE:
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
        break;
    case YELLOW:
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
        break;
    case RED:
        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        break;
    }
}

// == GAME ==================================

static void render_grid(int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH])
{
    SDL_Rect outline = { GRID_X_OFFSET, GRID_Y_OFFSET, GRID_WIDTH, GRID_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderDrawRect(renderer, &outline);

    int draw_x, draw_y;
    for (int i = 0; i < GRID_CELL_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_CELL_WIDTH; j++)
        {
            if (grid[i][j])
            {
                draw_x = GRID_X_OFFSET + (j * CELL_SIZE);
                draw_y = GRID_Y_OFFSET + (i * CELL_SIZE);
                SDL_Rect fill_rect = { draw_x, draw_y, CELL_SIZE, CELL_SIZE };
                set_render_color(grid[i][j]);
                SDL_RenderFillRect(renderer, &fill_rect);
            }
        }
    }
}

/**
 * Checks to see if the pixel coordinates are outside of the grid.
 */
static int is_out_of_bounds(int x, int y)
{
    return (x + CELL_SIZE - GRID_X_OFFSET > GRID_WIDTH ||
            x < GRID_X_OFFSET ||
            y + CELL_SIZE - GRID_Y_OFFSET > GRID_HEIGHT);
}

/**
 * Checks to see if the tetronimo at the given coordinates fits in the
 * play area and does not overlap any cells in the grid.
 * 
 * @param tetronimo the tetronimo to check
 * @param new_x     the proposed x pixel position
 * @param new_y     the proposed y pixel position
 * @param grid      the current game grid state
 * @returns         1 if the position is valid, 0 otherwise
 */
static int is_position_valid(tetronimo *tetronimo, int new_x, int new_y, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH])
{
    int draw_x, draw_y, grid_x, grid_y;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (tetronimo->matrix[i][j])
            {
                draw_x = new_x + (j * CELL_SIZE);
                draw_y = new_y + (i * CELL_SIZE);
                grid_x = CONVERT_TO_X_GRID(draw_x);
                grid_y = CONVERT_TO_Y_GRID(draw_y);
                if (is_out_of_bounds(draw_x, draw_y) || grid[grid_y][grid_x])
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}

/**
 * Handles keyboard input. Updates the shape with the new coordinates and rotation.
 */
static void handle_keys(SDL_Keycode key_code, shape *shape, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH])
{
    switch (key_code)
    {
    case SDLK_DOWN:
        shape->y += is_position_valid(shape->tetronimo, shape->x, shape->y + CELL_SIZE, grid) ? CELL_SIZE : 0;
        break;
    case SDLK_LEFT:
        shape->x -= is_position_valid(shape->tetronimo, shape->x - CELL_SIZE, shape->y, grid) ? CELL_SIZE : 0;
        break;
    case SDLK_RIGHT:
        shape->x += is_position_valid(shape->tetronimo, shape->x + CELL_SIZE, shape->y, grid) ? CELL_SIZE : 0;
        break;
    case SDLK_x:
        rotate(shape->tetronimo, NINETY_DEGREES);
        if (!is_position_valid(shape->tetronimo, shape->x, shape->y, grid))
        {
            rotate(shape->tetronimo, TWO_SEVENTY_DEGREES);
        }
        break;
    case SDLK_z:
        rotate(shape->tetronimo, TWO_SEVENTY_DEGREES);
        if (!is_position_valid(shape->tetronimo, shape->x, shape->y, grid))
        {
            rotate(shape->tetronimo, NINETY_DEGREES);
        }
        break;
    }
}

static void render_shape(shape *shape)
{
    int draw_x, draw_y;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (shape->tetronimo->matrix[i][j])
            {
                draw_x = shape->x + (j * CELL_SIZE);
                draw_y = shape->y + (i * CELL_SIZE);
                SDL_Rect fill_rect = { draw_x, draw_y, CELL_SIZE, CELL_SIZE };
                set_render_color(shape->color);
                SDL_RenderFillRect(renderer, &fill_rect);
            }
        }
    }
}

/**
 * Removes row from the grid if it is full.
 */
static void remove_full_row(int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], int row)
{
    int count = 0;
    for (int col = 0; col < GRID_CELL_WIDTH; col++)
    {
        if (grid[row][col])
        {
            count++;
        }
    }

    if (count == GRID_CELL_WIDTH)
    {
        // Shift everything in the grid down from row up to the second row
        for (int r = row; r > 0; r--)
        {
            for (int col = 0; col < GRID_CELL_WIDTH; col++)
            {
                grid[r][col] = grid[r - 1][col];
            }
        }

        // Clear out the top row
        for (int col = 0; col < GRID_CELL_WIDTH; col++)
        {
            grid[0][col] = 0;
        }
    }
}

/**
 * Adds the tetronimo to the playing area
 */
static void add_to_grid(int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], shape *shape)
{
    int grid_x, grid_y;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        grid_y = CONVERT_TO_Y_GRID(shape->y + (i * CELL_SIZE));

        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (shape->tetronimo->matrix[i][j])
            {
                grid_x = CONVERT_TO_X_GRID(shape->x + (j * CELL_SIZE));
                grid[grid_y][grid_x] = shape->color;
            }
        }

        remove_full_row(grid, grid_y);
    }
}

/**
 * Sets up a new random shape at the top of the screen with a random color
 */
static void reset_shape(shape *shape)
{
    shape->tetronimo = &tetronimoes[rand() % NUM_TETRONIMOES];
    if (shape->tetronimo->direction != NONE && shape->tetronimo->direction != UP)
    {
        rotate(shape->tetronimo, 4 - shape->tetronimo->direction);
    }

    shape->color = (rand() % YELLOW) + 1;
    shape->x = (GRID_WIDTH / 2) + GRID_X_OFFSET;
    shape->y = GRID_Y_OFFSET;
}

int main()
{
    if (init())
    {
        return 1;
    }

    srand(time(0));

    int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH] = { 0 };
    shape shape;
    reset_shape(&shape);
    SDL_Event e;
    int quit = 0;
    int speed = 90;
    int loopCnt = 0;
    int running = 1;
    int num_pieces = 1;
    uint32_t start_ms;
    while (!quit)
    {
        start_ms = SDL_GetTicks();
        loopCnt += running;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                if (running)
                {
                    handle_keys(e.key.keysym.sym, &shape, grid);
                }
                break;
            }
        }

        // Force tetronimo down after a time
        if (loopCnt == speed)
        {
            loopCnt = 0;
            if (is_position_valid(shape.tetronimo, shape.x, shape.y + CELL_SIZE, grid))
            {
                shape.y += CELL_SIZE;
            }
            else
            {
                add_to_grid(grid, &shape);
                reset_shape(&shape);
                num_pieces++;
                if (!is_position_valid(shape.tetronimo, shape.x, shape.y, grid))
                {
                    shape.color = RED;
                    running = 0;
                }

                // Increase speed / difficulty
                if (num_pieces % 10 == 0 && speed > 10)
                {
                    speed -= 10;
                    printf("New speed: %d, Num Pieces: %d\n", speed, num_pieces);
                }
            }
        }

        // Initialize renderer color & clear screen
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        render_grid(grid);
        render_shape(&shape);

        // Update screen
        SDL_RenderPresent(renderer);

        // Limit FPS to avoid maxing out CPU
        int frameTicks = SDL_GetTicks() - start_ms;
        if (frameTicks < SCREEN_TICKS_PER_FRAME)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    close();
    return 0;
}
