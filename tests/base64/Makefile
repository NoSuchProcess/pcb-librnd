ROOT=../..
LIBRND=$(ROOT)/src/librnd/core

CFLAGS = -Wall -g -I$(ROOT)/src

all: main

test: main
	@./main && echo "*** QC PASS ***"

main: main.o $(LIBRND)/base64.o
	$(CC) $(LDFLAGS) -o main   main.o $(LIBRND)/base64.o

main.o: main.c
	$(CC) -o main.o -c $(CFLAGS) main.c
