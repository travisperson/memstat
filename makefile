SRC=src
CC=clang

all: curses

curses: $(SRC)/memstat.c
	$(CC) -g -Wall $(SRC)/memstat.c -lcurses -o memstat
