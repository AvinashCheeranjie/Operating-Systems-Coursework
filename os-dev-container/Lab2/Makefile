CC = gcc
CFLAGS = -pthread -O2 -Wall -Wextra
SRC1 = lab2part1.c
SRC2=lab2part2.c

all: lab2part1 lab2part2

lab2part1: 
	$(CC) $(SRC1) $(CFLAGS) -o $@ $^

lab2part2: 
	$(CC) $(SRC2) $(CFLAGS) -o $@ $^

clean:
	rm -rf lab2part1 lab2part2

.PHONY: all clean