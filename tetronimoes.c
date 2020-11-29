/**
 * Rotate a tetronimo 90, 180 or 270 degrees
 */

#include <stdio.h>
#include "tetronimoes.h"

tetronimo tetronimoes[NUM_TETRONIMOES] = {
    {
        { 0, 1, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 1, 1, 0 },
        { 0, 0, 0, 0 }
    },
    {
        { 0, 0, 1, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 1, 0 }
    },
    {
        { 0, 0, 0, 0 },
        { 0, 1, 1, 0 },
        { 0, 1, 1, 0 },
        { 0, 0, 0, 0 }
    },
    {
        { 0, 0, 1, 0 },
        { 0, 1, 1, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 0 }
    },
    {
        { 0, 1, 0, 0 },
        { 0, 1, 1, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 0 }
    }
};

static void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

static void transpose_square(tetronimo tetronimo)
{
    for (int y = 0; y < MATRIX_SIZE; y++)
    {
        for (int x = y + 1; x < MATRIX_SIZE; x++)
        {
            swap(&tetronimo[x][y], &tetronimo[y][x]);
        }
    }
}

static void reverse_rows(tetronimo tetronimo)
{
    for (int y = 0; y < MATRIX_SIZE; y++)
    {
        for (int x = 0; x < MATRIX_SIZE / 2; x++)
        {
            swap(&tetronimo[y][x], &tetronimo[y][MATRIX_SIZE - 1 - x]);
        }
    }
}

static void reverse_cols(tetronimo tetronimo)
{
    for (int y = 0; y < MATRIX_SIZE / 2; y++)
    {
        for (int x = 0; x < MATRIX_SIZE; x++)
        {
            swap(&tetronimo[y][x], &tetronimo[MATRIX_SIZE - 1 - y][x]);
        }
    }
}

/**
 * @brief Rotates a matrix in place
 * 
 * @param tetronimo a tetronimo, aka an MATRIX_SIZE x MATRIX_SIZE matrix
 * @param r rotation mode: 0 - 90; 1 - 180; 2 - 270
 * 
 * @see https://stackoverflow.com/a/8664879
 */
void rotate(tetronimo tetronimo, int r)
{
    switch (r)
    {
    case 0: /* 90 degrees */
        transpose_square(tetronimo);
        reverse_rows(tetronimo);
        break;
    case 1: /* 180 degrees */
        reverse_rows(tetronimo);
        reverse_cols(tetronimo);
        break;
    case 2: /* 270 degrees */
        reverse_rows(tetronimo);
        transpose_square(tetronimo);
        break;
    default:
        break;
    }
}
