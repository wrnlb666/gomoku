CC = gcc
WC = emcc
CFLAG = -Wall -Wextra -std=gnu17 -pedantic `pkg-config --cflags SDL2 SDL2_ttf SDL2_net SDL2_image`
WFLAG = -Wall -Wextra -std=gnu17 -pedantic -s WASM=1 -s USE_SDL=2 -s USE_SDL_TTF=2 -s -s USE_SDL_NET=2 -s USE_SDL_IMAGE=2 -sSDL2_IMAGE_FORMATS='["png"]' --preload-file .
LIB = `pkg-config --libs SDL2 SDL2_ttf SDL2_net SDL2_image` -lpthread
OBJ = 

all: main test wasm

main: main.c helper.c helper.h
#	$(CC) $(CFLAG) helper.c -c
	$(CC) $(CFLAG) $(OBJ) $< -o $@ $(LIB)

wasm: main.c helper.c helper.h
#	$(WC) $(WFLAG) helper.c -c
	$(WC) $(WFLAG) $(OBJ) $< -o index.html

test: main.c
#	$(CC) $(CFLAG) helper.c -c
	$(CC) $(CFLAG) -DTEST $(OBJ) $< -o $@ $(LIB)
