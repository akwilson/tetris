#ifndef TETRONIMOES_H
#define TETRONIMOES_H

#define MATRIX_SIZE 4
#define NUM_TETRONIMOES 5

typedef int tetronimo[MATRIX_SIZE][MATRIX_SIZE];

tetronimo tetronimoes[NUM_TETRONIMOES];

#define LOOKUP_TETRONIMO(x) &tetronimoes[x]

void rotate(tetronimo tetronimo, int r);

#endif