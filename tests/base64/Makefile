ROOT=../..
LIBRND=$(ROOT)/src/librnd/core

CFLAGS = -Wall -g -I$(ROOT)/src

all: main

test: main
	@./main && echo "*** QC PASS ***"

main: main.o $(LIBRND)/base64.o

main.o: main.c
