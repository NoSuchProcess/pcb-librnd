all: test_parse

ROOT=../..
include $(ROOT)/Makefile.conf
REM=$(ROOT)/src/librnd/plugins/hid_remote
CFLAGS = $(PCB_RND_C89FLAGS) -g -I$(REM) -I$(ROOT)/src -I$(ROOT)


test_parse: test_parse.o
	$(CC) $(LDFLAGS) -o test_parse test_parse.o  $(LDLIBS)

test_parse.o: test_parse.c $(REM)/proto_lowcommon.h $(REM)/proto_lowparse.h $(REM)/proto_lowdbg.h

test:
	@./test_all.sh

clean:
	-$(SCCBOX) rm -f test_parse test_parse.o
