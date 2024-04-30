SVGS= \
	point-in-poly.svg \
	v-test.svg

all: $(SVGS)

point-in-poly.svg: src/point-in-poly.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/point-in-poly.rp

v-test.svg: src/v-test.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/v-test.rp

