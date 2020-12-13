/**
 * Definition of tetronimoes and a function to rotate them
 */
#pragma once

// tetronimoes are defined in a 4 X 4 matrix
#define MATRIX_SIZE 4

/**
 * The direction the tetronimo is facing
 */
typedef enum direction
{
    NONE = -1, // tetronimo cannot be rotated
    UP,
    RIGHT,
    DOWN,
    LEFT
} direction;

/**
 * Valid tetronimo rotation options
 */
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

/**
 * Selects a random tetronimo from those available.
 */
tetronimo *get_random_tetronimo();

/**
 * Rotates a tetronimo in place and maintains a record if its direction
 * 
 * @param tetronimo the tetronimo to rotate
 * @param rotation the rotation direction
 * 
 * @see https://stackoverflow.com/a/8664879
 */
void rotate(tetronimo *tetronimo, rotation rotation);
