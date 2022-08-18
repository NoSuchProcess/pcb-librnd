ROOT=../..
include $(ROOT)/Makefile.conf

include $(LIBRND_MAK)

CFLAGS = $(PCB_RND_C89FLAGS) -I$(ROOT)/src -I$(ROOT)/src_3rd $(CFLAGS_LIBRND_FUNGW) $(CFLAGS_LIBRND)
LDLIBS = -lm $(LDFLAGS_LIBRND_FUNGW) $(LDFLAGS_LIBRND)

LIB_HIDLIB=-lrnd-hid -lrnd-3rd

HELPER_OBJS= conf_core.o

DEPH = \
	$(ROOT)/src/librnd/core/conf.h \
	$(ROOT)/src/librnd/core/conf_hid.h \
	$(ROOT)/src/librnd/core/compat_misc.h \
	conf_core.h diag_conf.h

OBJS = conftest.o help.o

all: conftest

conftest: $(OBJS) $(HELPER_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(HELPER_OBJS) $(LIB_HIDLIB) $(LDLIBS) -o conftest

conftest.o: conftest.c $(DEPH)
	$(CC) $(CFLAGS) -c conftest.c -o conftest.o

help.o: help.c

$(LIB_HIDLIB):
	cd ../../src && make

test:
	cd tests && $(MAKE) all

clean:
	-$(SCCBOX) rm -f conftest tests/*.out ../../src_plugins/diag/diag_conf.o $(OBJS)
