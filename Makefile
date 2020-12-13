SRCS = graphics.c tetris.c tetronimoes.c

BIN1 = tetris
BIN1_SRCS = $(SRCS)
LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf

WASM1 = tetris.js
WASM1_SRCS = $(SRCS)
WASM1_ASSETS = assets
WASM1_USE_SDL2 = Y

include lib/simplified-make/simplified.mk
