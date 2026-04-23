#   make          – compile the program (default)
#   make run      – compile and run
#   make clean    – remove the compiled binary

CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -g
TARGET = fs_sim
SRCS   = fs_indexed.c main.c

all: $(SRCS) fs_indexed.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

run: all
	./$(TARGET)

clean:
	rm -rf $(TARGET)

.PHONY: all run clean