CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lGL -lGLU -lglut -lm

SRC = main.c player.c referee.c team.c
OBJ = $(SRC:.c=.o)
EXEC = rope_game

.PHONY: all clean run

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

run: all
	./$(EXEC) config.txt