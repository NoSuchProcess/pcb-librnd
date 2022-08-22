ROOT=../..
include $(ROOT)/Makefile.conf
include $(ROOT)/src/librnd/core/librnd.mak

CFLAGS = $(PCB_RND_C89FLAGS) -I$(ROOT)/src -I$(ROOT)/src_3rd $(CFLAGS_LIBRND_FUNGW) $(CFLAGS_LIBRND)
LDLIBS = -lm $(LDFLAGS_LIBRND_FUNGW) $(LDFLAGS_LIBRND)

HELPER_OBJS=conf_core.o
LIBRND_OBJS= \
 $(ROOT)/src/librnd/plugins/diag_rnd/diag_rnd.o \
 $(ROOT)/src/librnd-hid.a $(ROOT)/src/librnd-3rd.a

DEPH = \
	$(ROOT)/src/librnd/core/conf.h \
	$(ROOT)/src/librnd/core/conf_hid.h \
	$(ROOT)/src/librnd/core/compat_misc.h \
	conf_core.h diag_conf.h

OBJS = conftest.o help.o

all: conftest

conftest: $(OBJS) $(HELPER_OBJS) $(LIBRND_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(HELPER_OBJS) $(LIBRND_OBJS) $(LDLIBS) -o conftest

conftest.o: conftest.c $(DEPH)
	$(CC) $(CFLAGS) -c conftest.c -o conftest.o

help.o: help.c

$(ROOT)/src/librnd/plugins/diag_rnd/diag_rnd.o:  $(ROOT)/src/librnd/plugins/diag_rnd/diag_rnd.c

$(LIBRND_OBJS):
	cd ../../src && make

test:
	cd tests && $(MAKE) all

clean:
	-$(SCCBOX) rm -f conftest tests/*.out $(OBJS) $(HELPER_OBJS)
