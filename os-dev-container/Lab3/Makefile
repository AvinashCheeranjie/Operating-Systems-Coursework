CC = gcc
CFLAGS = -pthread -O2 -Wall -Wextra
SRC1 = lab3part1.c
SRC2=lab3part2.c

all: lab3part1 lab3part2

lab3part1: 
	$(CC) $(SRC1) $(CFLAGS) -o $@ $^

lab3part2: 
	$(CC) $(SRC2) $(CFLAGS) -o $@ $^

clean:
	rm -rf lab3part1 lab3part2

.PHONY: all clean