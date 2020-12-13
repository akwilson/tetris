#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "tetronimoes.h"
#include "graphics.h"

#define CELL_SIZE 25
#define GRID_X_OFFSET 50
#define GRID_Y_OFFSET 50
#define GRID_CELL_WIDTH 12
#define GRID_CELL_HEIGHT 18
#define GRID_WIDTH GRID_CELL_WIDTH * CELL_SIZE
#define GRID_HEIGHT GRID_CELL_HEIGHT * CELL_SIZE
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME (1000 / SCREEN_FPS)
#define INITIAL_SPEED 90
#define BTN_SPRITE_WIDTH 125
#define BTN_SPRITE_HEIGHT 40

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

enum images { BUTTON_SHEET, GAME_OVER };
enum sprites { PAUSE, RESTART, PAUSE_MO, RESTART_MO };

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
    int images[2];
    SDL_Rect **btn_sprites;
} game_state;

/**
 * Required for emscripten compatability.
 */
typedef struct game_data
{
    graphics *graphics;
    int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH];
    shape shape;
    game_state state;
    SDL_Event e;
    uint32_t start_ms;
    int quit;
} game_data;

// Macros to convert pixel positions to positions in the grid array
#define CONVERT_TO_X_GRID(x) (x - GRID_X_OFFSET) / CELL_SIZE
#define CONVERT_TO_Y_GRID(y) (y - GRID_Y_OFFSET) / CELL_SIZE

/**
 * Renders the grid to the screen
 */
static void render_grid(graphics *graphics, int grid[GRID_CELL_HEIGHT][GRID_CELL_WIDTH])
{
    // Render outline
    render_quad(graphics, GRID_X_OFFSET - 1, GRID_Y_OFFSET - 1, GRID_WIDTH + 2, GRID_HEIGHT + 2, 0, DARK);

    // Render grid cells
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
 * Gets the current level from the game state
 */
static int get_level(game_state *state)
{
    return ((INITIAL_SPEED - state->speed) / 10) + 1;
}

static int is_in_area(int area_x, int area_y, int width, int height, int x, int y)
{
    return (x >= area_x && x <= area_x + width && y >= area_y && y <= area_y + height);
}

/**
 * Render game status information -- score, game over message, buttons.
 */
static void render_ui(graphics *graphics, game_state *state)
{
    char message[512];
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Level
    sprintf(message, "Level %d", get_level(state));
    render_message(graphics, message, GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET);

    // Score
    sprintf(message, "Score %d", state->score);
    render_message(graphics, message, GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET * 2);

    // Horizontal line
    render_line(graphics, GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET * 3, 375);

    // Buttons
    int place_x = GRID_WIDTH + GRID_X_OFFSET * 2;
    int place_y = GRID_Y_OFFSET * 4;
    int s = is_in_area(place_x, place_y, BTN_SPRITE_WIDTH, BTN_SPRITE_HEIGHT, mouse_x, mouse_y) ? PAUSE_MO : PAUSE;
    render_image(graphics, state->images[BUTTON_SHEET],
                 place_x, place_y, state->btn_sprites[s]);

    place_x = GRID_WIDTH + BTN_SPRITE_WIDTH + GRID_X_OFFSET * 3;
    s = is_in_area(place_x, place_y, BTN_SPRITE_WIDTH, BTN_SPRITE_HEIGHT, mouse_x, mouse_y) ? RESTART_MO : RESTART;
    render_image(graphics, state->images[BUTTON_SHEET],
                 place_x, place_y, state->btn_sprites[s]);

    // Game over
    if (!state->running)
    {
        render_image(graphics, state->images[GAME_OVER], GRID_WIDTH + GRID_X_OFFSET * 2, GRID_Y_OFFSET * 5, 0);
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
    shape->tetronimo = get_random_tetronimo();
    if (shape->tetronimo->direction != NONE && shape->tetronimo->direction != UP)
    {
        rotate(shape->tetronimo, 4 - shape->tetronimo->direction);
    }

    shape->color = (rand() % BLUE) + 1;
    shape->x = (GRID_WIDTH / 2) + GRID_X_OFFSET;
    shape->y = GRID_Y_OFFSET;
}

static void init_game(game_state *state)
{
    state->num_pieces = 1;
    state->running = 1;
    state->speed = INITIAL_SPEED;
}

static int load_images(game_state *state, graphics *graphics)
{
    // Load button sprite sheet
    state->images[BUTTON_SHEET] = load_image(graphics, "assets/tetris_button_sheet.png");
    if (state->images[BUTTON_SHEET] < 0)
    {
        return 1;
    }

    // Define sprites
    state->btn_sprites = calloc(4, sizeof(SDL_Rect*));
    for (int i = 0; i <= RESTART_MO; i++)
    {
        state->btn_sprites[i] = calloc(1, sizeof(SDL_Rect));
        state->btn_sprites[i]->x = 0;
        state->btn_sprites[i]->y = BTN_SPRITE_HEIGHT * i;
        state->btn_sprites[i]->w = BTN_SPRITE_WIDTH;
        state->btn_sprites[i]->h = BTN_SPRITE_HEIGHT;
    }

    // Load game over image
    state->images[GAME_OVER] = load_image(graphics, "assets/tetris_go.png");
    if (state->images[GAME_OVER] < 0)
    {
        return 1;
    }

    return 0;
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

static void cleanup(game_state *state)
{
    for (int i = 0; i <= RESTART_MO; i++)
    {
        free(state->btn_sprites[i]);
    }

    free(state->btn_sprites);

#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
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

static void main_loop(void *g_data)
{
    game_data *data = g_data;
    data->start_ms = SDL_GetTicks();
    new_frame(&data->state);

    while (SDL_PollEvent(&data->e))
    {
        switch (data->e.type)
        {
        case SDL_QUIT:
            data->quit = 1;
            break;
        case SDL_KEYDOWN:
            handle_keys(data->e.key.keysym.sym, &data->shape, data->grid, &data->state);
            break;
        }
    }

    if (check_force_down(&data->state))
    {
        if (is_position_valid(data->shape.tetronimo, data->shape.x, data->shape.y + CELL_SIZE, data->grid))
        {
            data->shape.y += CELL_SIZE;
        }
        else
        {
            end_shape(&data->state, data->grid, &data->shape);
        }
    }

    clear_frame(data->graphics);

    render_grid(data->graphics, data->grid);
    render_shape_cells(data->graphics, &data->shape);
    render_ui(data->graphics, &data->state);

    commit_to_screen(data->graphics);

    // Limit FPS to avoid maxing out CPU
    int frameTicks = SDL_GetTicks() - data->start_ms;
    if (frameTicks < SCREEN_TICKS_PER_FRAME)
    {
        SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
    }
}

int main()
{
    game_data game_data = { 0 };
    game_data.graphics = init_graphics();
    if (!game_data.graphics)
    {
        return 1;
    }

    srand(time(0));

    if (load_images(&game_data.state, game_data.graphics))
    {
        return 1;
    }

    init_game(&game_data.state);
    reset_shape(&game_data.shape);

    while (!game_data.quit)
    {
#ifdef __EMSCRIPTEN__
        emscripten_set_main_loop_arg(main_loop, &game_data, 0, 1);
#else
        main_loop(&game_data);
#endif
    }

    close_graphics(game_data.graphics);
    cleanup(&game_data.state);
    return 0;
}
