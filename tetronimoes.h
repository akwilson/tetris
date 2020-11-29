#ifndef TETRONIMOES_H
#define TETRONIMOES_H

#define MATRIX_SIZE 4

typedef int tetronimo[MATRIX_SIZE][MATRIX_SIZE];

tetronimo tetronimoes[5];

#define LOOKUP_TETRONIMO(x) &tetronimoes[x]

void rotate(tetronimo tetronimo, int r);

#endif