CC = gcc
CFLAG = -Wall -Wextra `pkg-config --cflags SDL2 SDL2_ttf bdw-gc libzmq`
LIB = `pkg-config --libs SDL2 SDL2_ttf bdw-gc libzmq`
OBJ = 


main: main.c
	$(CC) $(CFLAG) $(OBJ) $< -o $@ $(LIB)