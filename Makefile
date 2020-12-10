BIN1 = tetris
BIN1_SRCS = graphics.c tetris.c tetronimoes.c
LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf

include lib/simplified.mk
