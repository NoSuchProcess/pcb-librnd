ROOT=../../..
include $(ROOT)/Makefile.conf

TESTS =  \
	list_merge.diff \
	list_set.diff \
	arr_merge.diff \
	arr_set.diff \
	scalar.diff

.SUFFIXES: .ref .test .out .diff .REF

all: $(TESTS)
	@echo "conf:                *** QC PASS ***"

.test.out:
	@../conftest < $*.test >$*.out

.test.REF:
	@../conftest < $*.test >$*.ref
	@echo $*.ref generated, please validate the content

.out.diff:
	@diff -u $*.ref $*.out


