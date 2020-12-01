/**
 * Rotate a tetronimo 90, 180 or 270 degrees
 */

#include <stdio.h>
#include "tetronimoes.h"

tetronimo tetronimoes[NUM_TETRONIMOES] = {
    {
        {
            { 0, 1, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 1, 1, 0 },
            { 0, 0, 0, 0 }
        },
        UP
    },
    {
        {
            { 0, 0, 1, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 1, 0 }
        },
        UP
    },
    {
        {
            { 0, 1, 1, 0 },
            { 0, 1, 1, 0 },
            { 0, 0, 0, 0 },
            { 0, 0, 0, 0 }
        },
        NONE
    },
    {
        {
            { 0, 0, 1, 0 },
            { 0, 1, 1, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 0 }
        },
        UP
    },
    {
        {
            { 0, 1, 0, 0 },
            { 0, 1, 1, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 0 }
        },
        UP
    }
};

static void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static void transpose_square(int matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    for (int y = 0; y < MATRIX_SIZE; y++)
    {
        for (int x = y + 1; x < MATRIX_SIZE; x++)
        {
            swap(&matrix[x][y], &matrix[y][x]);
        }
    }
}

static void reverse_rows(int matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    for (int y = 0; y < MATRIX_SIZE; y++)
    {
        for (int x = 0; x < MATRIX_SIZE / 2; x++)
        {
            swap(&matrix[y][x], &matrix[y][MATRIX_SIZE - 1 - x]);
        }
    }
}

static void reverse_cols(int matrix[MATRIX_SIZE][MATRIX_SIZE])
{
    for (int y = 0; y < MATRIX_SIZE / 2; y++)
    {
        for (int x = 0; x < MATRIX_SIZE; x++)
        {
            swap(&matrix[y][x], &matrix[MATRIX_SIZE - 1 - y][x]);
        }
    }
}

/**
 * @brief Rotates a tetronimo and maintains a record if its direction
 * 
 * @see https://stackoverflow.com/a/8664879
 */
void rotate(tetronimo *tetronimo, rotation rotation)
{
    if (tetronimo->direction == NONE)
    {
        return;
    }

    switch (rotation)
    {
    case NINETY_DEGREES:
        transpose_square(tetronimo->matrix);
        reverse_rows(tetronimo->matrix);
        break;
    case ONE_EIGHTY_DEGREES:
        reverse_rows(tetronimo->matrix);
        reverse_cols(tetronimo->matrix);
        break;
    case TWO_SEVENTY_DEGREES:
        reverse_rows(tetronimo->matrix);
        transpose_square(tetronimo->matrix);
        break;
    default:
        break;
    }

    tetronimo->direction = (tetronimo->direction + rotation) % 4;
}
