CC = gcc
WC = emcc
CFLAG = -Wall -Wextra -std=gnu17 -pedantic `pkg-config --cflags SDL2 SDL2_ttf SDL2_net`
WFLAG = -Wall -Wextra -std=gnu17 -pedantic -s WASM=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s -s USE_SDL_NET=2 --preload-file .
LIB = `pkg-config --libs SDL2 SDL2_ttf SDL2_net` -lpthread
OBJ = helper.o

all: helper main

main: main.c helper.c helper.h
	$(CC) $(CFLAG) $(OBJ) $< -o $@ $(LIB)

wasm: main.c
	$(WC) $(WFLAG) -DTEST $< -o index.html

helper: helper.c helper.h
	$(CC) $(CFLAG) $< -c

test: main.c
	$(CC) $(CFLAG) -DTEST $(OBJ) $< -o $@ $(LIB)
