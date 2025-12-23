CC=clang
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic
LDFLAGS=-lncurses

SRC=$(wildcard src/*.c src/game/*.c src/ui/*.c)
OBJ=$(SRC:.c=.o)

tetonor: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJ) tetonor
