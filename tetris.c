#include <stdlib.h>
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

// The window we'll be rendering to
static SDL_Window *window;

// The renderer to draw the texture on the window
static SDL_Renderer *renderer;

typedef struct shape
{
    int tetronimo;
    int color;
    int x;
    int y;
} shape;

static int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH] = {0};

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

// == GAME ==================================

static void draw_grid()
{
    SDL_Rect outline = { GRID_X_OFFSET, GRID_Y_OFFSET, GRID_WIDTH, GRID_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderDrawRect(renderer, &outline);
}

static int is_out_of_bounds(int x, int y)
{
    return (x + CELL_SIZE - GRID_X_OFFSET > GRID_WIDTH ||
            x < GRID_X_OFFSET ||
            y + CELL_SIZE - GRID_Y_OFFSET > GRID_HEIGHT);
}

static int is_position_valid(tetronimo tetronimo, int newX, int newY)
{
    int drawX, drawY;
    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if (tetronimo[i][j])
            {
                drawX = newX + (j * CELL_SIZE);
                drawY = newY + (i * CELL_SIZE);
                if (is_out_of_bounds(drawX, drawY))
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}

static void handle_keys(SDL_Keycode key_code, shape *shape)
{
    tetronimo *c_tet = LOOKUP_TETRONIMO(shape->tetronimo);
    switch (key_code)
    {
    case SDLK_DOWN:
        shape->y += is_position_valid(*c_tet, shape->x, shape->y + CELL_SIZE) ? CELL_SIZE : 0;
        break;
    case SDLK_LEFT:
        shape->x -= is_position_valid(*c_tet, shape->x - CELL_SIZE, shape->y) ? CELL_SIZE : 0;
        break;
    case SDLK_RIGHT:
        shape->x += is_position_valid(*c_tet, shape->x + CELL_SIZE, shape->y) ? CELL_SIZE : 0;
        break;
    case SDLK_x:
        rotate(*c_tet, 0);
        if (!is_position_valid(*c_tet, shape->x, shape->y))
        {
            rotate(*c_tet, 2);
        }
        break;
    case SDLK_z:
        rotate(*c_tet, 2);
        if (!is_position_valid(*c_tet, shape->x, shape->y))
        {
            rotate(*c_tet, 0);
        }
        break;
    }
}

static void render_shape(shape *shape)
{
    int drawX, drawY;
    tetronimo *c_tet = LOOKUP_TETRONIMO(shape->tetronimo);

    for (int i = 0; i < MATRIX_SIZE; i++)
    {
        for (int j = 0; j < MATRIX_SIZE; j++)
        {
            if ((*c_tet)[i][j])
            {
                drawX = shape->x + (j * CELL_SIZE);
                drawY = shape->y + (i * CELL_SIZE);
                SDL_Rect fillRect = { drawX, drawY, CELL_SIZE, CELL_SIZE };
                SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
                SDL_RenderFillRect(renderer, &fillRect);
            }
        }
    }
}

int main()
{
    if (init())
    {
        return 1;
    }

    shape shape = { 0 };
    shape.x = (GRID_WIDTH / 2) + GRID_X_OFFSET;
    shape.y = GRID_Y_OFFSET;
    SDL_Event e;
    int quit = 0;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                handle_keys(e.key.keysym.sym, &shape);
                break;
            }
        }

        // Initialize renderer color & clear screen
        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderClear(renderer);

        draw_grid();
        render_shape(&shape);

        // Update screen
        SDL_RenderPresent(renderer);
    }

    close();
    return 0;
}
