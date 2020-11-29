BIN1 = tetris
BIN1_SRCS = tetris.c tetronimoes.c

BUILDDIR = build
LIB_PATH = /usr/local/lib
LIBS = -lSDL2

include lib/simplified.mk
