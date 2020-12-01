#ifndef TETRONIMOES_H
#define TETRONIMOES_H

// tetronimoes are defined in a 4 X 4 matrix
#define MATRIX_SIZE 4
#define NUM_TETRONIMOES 7

typedef enum direction
{
    NONE = -1, // tetronimo cannot be rotated
    UP,
    RIGHT,
    DOWN,
    LEFT
} direction;

typedef enum rotation
{
    NINETY_DEGREES = 1,
    ONE_EIGHTY_DEGREES,
    TWO_SEVENTY_DEGREES
} rotation;

/**
 * A tetris piece.
 */
typedef struct tetronimo
{
    int matrix[MATRIX_SIZE][MATRIX_SIZE]; // The shape of the tetronimo
    direction direction;                  // The tetronimo's current direction
} tetronimo;

tetronimo tetronimoes[NUM_TETRONIMOES];

void rotate(tetronimo *tetronimo, rotation rotation);

#endif