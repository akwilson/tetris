#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "graphics.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define IMAGE_COUNT 5

struct image
{
    SDL_Texture *texture;
    int width;
    int height;
};

struct graphics
{
    SDL_Window *window;     // The window we'll be rendering to
    SDL_Renderer *renderer; // The renderer to draw the texture on the window
    TTF_Font *font;         // Font for displaying text
    struct image **images;   // Loaded images
};

/**
 * Loads the TTF font at the specified path
 */
static TTF_Font *load_font(const char *path)
{
    TTF_Font *ttf_font = TTF_OpenFont(path, 30);
    if (!ttf_font)
    {
        fprintf(stderr, "Failed to load font. SDL_ttf Error: %s\n", TTF_GetError());
        return 0;
    }

    return ttf_font;
}

/**
 * Set the renderer up to draw using one of the pre-defined colors
 */
static void set_render_color(graphics *graphics, color color)
{
    switch (color)
    {
    case BLACK:
        SDL_SetRenderDrawColor(graphics->renderer, 0x00, 0x00, 0x00, 0xFF);
        break;
    case YELLOW:
        SDL_SetRenderDrawColor(graphics->renderer, 0xEB, 0xCB, 0x8B, 0xFF);
        break;
    case GREEN:
        SDL_SetRenderDrawColor(graphics->renderer, 0xA3, 0xBE, 0x8C, 0xFF);
        break;
    case PINK:
        SDL_SetRenderDrawColor(graphics->renderer, 0xB4, 0x8E, 0xAD, 0xFF);
        break;
    case BLUE:
        SDL_SetRenderDrawColor(graphics->renderer, 0x5E, 0x81, 0xAC, 0xFF);
        break;
    case RED:
        SDL_SetRenderDrawColor(graphics->renderer, 0xBF, 0x61, 0x6A, 0xFF);
        break;
    case DARK:
        SDL_SetRenderDrawColor(graphics->renderer, 0x4C, 0x56, 0x6A, 0xFF);
        break;
    }
}

/**
 * Render some text to the screen
 */
static int render_text_texture(graphics *graphics, const char *message, int x, int y)
{
    // Render text
    SDL_Color text_color = { 0xEC, 0xEF, 0xF4, 0xFF };
    SDL_Color back_color = { 0x2E, 0x34, 0x40, 0xFF };
    SDL_Surface *text_surface = TTF_RenderText_Shaded(graphics->font, message, text_color, back_color);
    if (!text_surface)
    {
        fprintf(stderr, "Unable to render text surface. SDL_ttf Error: %s\n", TTF_GetError());
        return 1;
    }

    // Create texture from surface pixels
    SDL_Texture *text_text = SDL_CreateTextureFromSurface(graphics->renderer, text_surface);
    if (!text_text)
    {
        fprintf(stderr, "Unable to create texture from rendered text. SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    // Render the texture
    SDL_Rect render_quad = { x, y, text_surface->w, text_surface->h };
    if (SDL_RenderCopy(graphics->renderer, text_text, 0, &render_quad))
    {
        fprintf(stderr, "Unable to render text. SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(text_text);

    return 0;
}

graphics *init_graphics()
{
    // Initialise SDL and the SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL init failed. SDL_Error:%s\n", SDL_GetError());
        return 0;
    }

    // Create window
    SDL_Window *window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        fprintf(stderr, "Window could not be created. SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Create renderer for window
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "Renderer could not be created. SDL Error: %s\n", SDL_GetError());
        return 0;
    }

    // Initialize PNG loading
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags))
    {
        fprintf(stderr, "SDL_image could not initialize. SDL_image Error: %s\n", IMG_GetError());
        return 0;
    }

    // Initialize TTF Font loading
    if (TTF_Init() == -1)
    {
        fprintf(stderr, "SDL_ttf could not initialize. SDL_ttf Error: %s\n", TTF_GetError());
        return 0;
    }

    graphics *graphics = calloc(1, sizeof(struct graphics));
    graphics->window = window;
    graphics->renderer = renderer;
    graphics->images = calloc(IMAGE_COUNT, sizeof(struct image*));

    return graphics;
}

void clear_frame(graphics *graphics)
{
    SDL_SetRenderDrawColor(graphics->renderer, 0x2E, 0x34, 0x40, 0xFF);
    SDL_RenderClear(graphics->renderer);
}

void render_quad(graphics *graphics, int x, int y, int width, int height, int filled, color color)
{
    SDL_Rect outline = { x, y, width, height };
    set_render_color(graphics, color);
    if (filled)
    {
        SDL_RenderFillRect(graphics->renderer, &outline);
    }
    else
    {
        SDL_RenderDrawRect(graphics->renderer, &outline);
    }
}

void render_line(graphics *graphics, int x, int y, int l)
{
    set_render_color(graphics, DARK);
    SDL_RenderDrawLine(graphics->renderer, x, y, x + l, y);
}

void render_message(graphics *graphics, char* message, int x, int y)
{
    if (!graphics->font)
    {
        graphics->font = load_font("assets/Arial.ttf");
    }

    render_text_texture(graphics, message, x, y);
}

void commit_to_screen(graphics *graphics)
{
    SDL_RenderPresent(graphics->renderer);
}

int load_image(graphics *graphics, const char *path)
{
    SDL_Surface *loaded_surface = IMG_Load(path);
    if (!loaded_surface)
    {
        fprintf(stderr, "Unable to load image. SDL_image Error: %s\n", IMG_GetError());
        return -1;
    }

    // Color key image. Specify a pixel that will be treated as transparent, in this case cyan.
    //SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

    // Create texture from surface pixels
    SDL_Texture *image_texture = SDL_CreateTextureFromSurface(graphics->renderer, loaded_surface);
    if (!image_texture)
    {
        fprintf(stderr, "Unable to create texture from %s. SDL Error: %s\n", path, SDL_GetError());
        return -1;
    }

    struct image *img = malloc(sizeof(struct image));
    img->texture = image_texture;
    img->width = loaded_surface->w;
    img->height = loaded_surface->h;

    SDL_FreeSurface(loaded_surface);

    // Find unused handle
    for (int i = 0; i < IMAGE_COUNT; i++)
    {
        if (graphics->images[i] == 0)
        {
            graphics->images[i] = img;
            return i;
        }
    }

    return -1;
}

void render_image(graphics *graphics, int handle, int x, int y, SDL_Rect *sprite)
{
    // TODO: some error handling needed here
    struct image *img = graphics->images[handle];

    // Set rendering space and render to screen
    SDL_Rect renderQuad = { x, y, img->width, img->height };
    if (sprite)
    {
        img->width = sprite->w;
        img->height = sprite->h;
    }
    
    SDL_RenderCopy(graphics->renderer, img->texture, sprite, &renderQuad);
}

void close_graphics(graphics *graphics)
{
    // Destroy window and renderer
    SDL_DestroyRenderer(graphics->renderer);
    SDL_DestroyWindow(graphics->window);
    if (graphics->font)
    {
        TTF_CloseFont(graphics->font);
        TTF_Quit();
    }

    if (graphics->images)
    {
        for (int i = 0; i < IMAGE_COUNT; i++)
        {
            if (graphics->images[i])
            {
                SDL_DestroyTexture(graphics->images[i]->texture);
                free(graphics->images[i]);
                graphics->images[i] = 0;
            }
        }

        free(graphics->images);
    }

    free(graphics);

    // Quit SDL subsys
    SDL_Quit();
}
