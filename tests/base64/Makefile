ROOT=../..
LIBRND=$(ROOT)/src/librnd/core
include $(ROOT)/Makefile.conf

CFLAGS = -Wall -g -I$(ROOT)/src $(PCB_RND_C89FLAGS)

all: main

test: main
	@./main && echo "*** QC PASS ***"

main: main.o $(LIBRND)/base64.o
	$(CC) $(LDFLAGS) -o main   main.o $(LIBRND)/base64.o

main.o: main.c
	$(CC) -o main.o -c $(CFLAGS) main.c

clean:
	-rm main main.o
