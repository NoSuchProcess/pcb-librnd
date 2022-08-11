TRUNK = ../..
LIBRND_ROOT=include/$(HL_LIBDIR)/..

include $(TRUNK)/Makefile.conf
include $(TRUNK)/src/librnd/core/librnd.mak


CFLAGS =  -I$(LIBRND_ROOT)/include -I$(LIBRND_ROOT)/include/librnd/src_3rd $(PCB_RND_C89FLAGS) $(CFLAGS_LIBRND)
LDFLAGS = $(LIBRND_RDYNAMIC) $(LDFLAGS_LIBRND)
LIBRND_A = include/$(HL_LIBDIR)/librnd-poly.a include/$(HL_LIBDIR)/librnd-hid.a include/$(HL_LIBDIR)/librnd-3rd.a

all: librnd_test librnd_includes

test: librnd_test librnd_includes
	./librnd_test --gui batch
	./librnd_includes

librnd_objs: $(LIBRND_A)
	-mkdir obj obj/poly obj/hid obj/3rd
	-rm obj/poly/*.o obj/hid/*.o obj/3rd/*.o
	cd obj/hid &&  ar x ../../include/$(HL_LIBDIR)/librnd-hid.a
	cd obj/poly && ar x ../../include/$(HL_LIBDIR)/librnd-poly.a
	cd obj/3rd &&  ar x ../../include/$(HL_LIBDIR)/librnd-3rd.a  


librnd_test: librnd_test.o librnd_objs
	$(CC) -o librnd_test librnd_test.o obj/hid/*.o obj/poly/*.o  obj/3rd/*.o $(LDFLAGS)


librnd_test.o: librnd_test.c includes
	$(CC) $(CFLAGS) -c -o librnd_test.o librnd_test.c

librnd_includes: librnd_includes.o librnd_objs
	$(CC) -o librnd_includes librnd_includes.o  obj/hid/*.o obj/poly/*.o  obj/3rd/*.o $(LDFLAGS)

librd_includes.o: librnd_includes.c includes

$(LIBRND) includes: ../../src/Makefile
	TDIR=`pwd` && cd ../../src && $(MAKE) && $(MAKE) install HOW="install -f" DESTDIR=$$TDIR/include PREFIX=

clean:
	-rm librnd_test librnd_test.o librnd_includes librnd_includes.o
	-TDIR=`pwd` && cd ../../src && $(MAKE) uninstall HOW="uninstall -f" DESTDIR=$$TDIR/include PREFIX=
	-rm -rf include/usr obj

distclean:
	-rm inc_all.h
