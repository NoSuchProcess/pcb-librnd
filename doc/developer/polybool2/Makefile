SVGS= \
	point-in-poly.svg

all: $(SVGS)

point-in-poly.svg: src/point-in-poly.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/point-in-poly.rp

