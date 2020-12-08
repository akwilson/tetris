#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "tetronimoes.h"
#include "graphics.h"

#define CELL_SIZE 25
#define GRID_X_OFFSET 25
#define GRID_Y_OFFSET 25
#define GRID_CELL_WIDTH 12
#define GRID_CELL_HEIGHT 18
#define GRID_WIDTH GRID_CELL_WIDTH * CELL_SIZE
#define GRID_HEIGHT GRID_CELL_HEIGHT * CELL_SIZE
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME (1000 / SCREEN_FPS)
#define INITIAL_SPEED 90

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

/**
 * Variables to control the state of the game
 */
typedef struct game_state
{
    int speed;
    int loop_count;
    int running;
    int num_pieces;
    int score;
} game_state;

// Macros to convert pixel positions to positions in the grid array
#define CONVERT_TO_X_GRID(x) (x - GRID_X_OFFSET) / CELL_SIZE
#define CONVERT_TO_Y_GRID(y) (y - GRID_Y_OFFSET) / CELL_SIZE

/**
 * Renders the grid to the screen
 */
static void render_grid_cells(graphics *graphics, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH])
{
    render_quad(graphics, GRID_X_OFFSET, GRID_Y_OFFSET, GRID_WIDTH, GRID_HEIGHT, 0, BLACK);

    int draw_x, draw_y;
    for (int i = 0; i < GRID_CELL_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_CELL_WIDTH; j++)
        {
            if (grid[i][j])
            {
                draw_x = GRID_X_OFFSET + (j * CELL_SIZE);
                draw_y = GRID_Y_OFFSET + (i * CELL_SIZE);
                render_quad(graphics, draw_x, draw_y, CELL_SIZE, CELL_SIZE, 1, grid[i][j]);
            }
        }
    }
}

/**
 * Renders the shape to the screen
 */
static void render_shape_cells(graphics *graphics, shape *shape)
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
                render_quad(graphics, draw_x, draw_y, CELL_SIZE, CELL_SIZE, 1, shape->color);
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
 * 
 * @param key_code the key pressed by the user
 * @param shape    the current in-play shape
 * @param grid     the play area
 * @param state    the game state
 */
static void handle_keys(SDL_Keycode key_code, shape *shape, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], game_state *state)
{
    if (!state->running)
    {
        return;
    }

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

/**
 * Removes row from the grid if it is full.
 * 
 * @returns 1 if the row was removed, 0 otherwise
 */
static int remove_full_row(int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], int row)
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

        return 1;
    }

    return 0;
}

/**
 * Gets the current level from the game state
 */
static int get_level(game_state *state)
{
    return ((INITIAL_SPEED - state->speed) / 10) + 1;
}

/**
 * Original Nintendo scoring system.
 */
static void update_score(game_state *state, int num_rows)
{
    static int SCORE_TABLE[5] = { 0, 40, 100, 300, 1200 };
    state->score += SCORE_TABLE[num_rows] * get_level(state);
}

/**
 * Adds the tetronimo to the playing area. Removes full rows and updates the game score.
 */
static void add_shape_to_grid(int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], shape *shape, game_state *state)
{
    int grid_x, grid_y, row_count = 0;
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

        row_count += remove_full_row(grid, grid_y);
    }

    update_score(state, row_count);
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

static void init_game(game_state *state)
{
    state->loop_count = 0;
    state->num_pieces = 1;
    state->running = 1;
    state->score = 0;
    state->speed = INITIAL_SPEED;
}

/**
 * Prepare the state for the next frame
 */
static void new_frame(game_state *state)
{
    state->loop_count += state->running;
}

/**
 * Checks if it is time to increase the level of difficulty, i.e. increase
 * the speed that tetronimoes drop down.
 */
static void check_level(game_state *state)
{
    if (state->num_pieces % 10 == 0 && state->speed > 10)
    {
        state->speed -= 10;
    }
}

/**
 * Checks if it is time to force the tetronimo down a row.
 */
static int check_force_down(game_state *state)
{
    if (state->loop_count == state->speed)
    {
        state->loop_count = 0;
        return 1;
    }

    return 0;
}

/**
 * End of life for a shape. Add it to the grid and select a new one.
 * Check for end of game and the level of difficulty.
 */
static void end_shape(game_state *state, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH], shape *shape)
{
    add_shape_to_grid(grid, shape, state);
    reset_shape(shape);
    state->num_pieces++;
    if (!is_position_valid(shape->tetronimo, shape->x, shape->y, grid))
    {
        shape->color = RED;
        state->running = 0;
    }

    check_level(state);
}

int main()
{
    graphics *graphics = init_graphics();
    if (!graphics)
    {
        return 1;
    }

    srand(time(0));

    int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH] = { 0 };
    shape shape;
    game_state state;
    SDL_Event e;
    uint32_t start_ms;
    int quit = 0;
    char message[512];

    init_game(&state);
    reset_shape(&shape);

    while (!quit)
    {
        start_ms = SDL_GetTicks();
        new_frame(&state);

        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                handle_keys(e.key.keysym.sym, &shape, grid, &state);
                break;
            }
        }

        if (check_force_down(&state))
        {
            if (is_position_valid(shape.tetronimo, shape.x, shape.y + CELL_SIZE, grid))
            {
                shape.y += CELL_SIZE;
            }
            else
            {
                end_shape(&state, grid, &shape);
            }
        }

        clear_frame(graphics);

        render_grid_cells(graphics, grid);
        render_shape_cells(graphics, &shape);

        sprintf(message, "Level: %d", get_level(&state));
        render_message(graphics, message, GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET);

        sprintf(message, "Score: %d", state.score);
        render_message(graphics, message, GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET * 2);

        commit_to_screen(graphics);

        // Limit FPS to avoid maxing out CPU
        int frameTicks = SDL_GetTicks() - start_ms;
        if (frameTicks < SCREEN_TICKS_PER_FRAME)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    close_graphics(graphics);
    return 0;
}
