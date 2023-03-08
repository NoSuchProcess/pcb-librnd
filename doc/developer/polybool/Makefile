SVGS= \
	pa_vect_inside_sect.svg \
	t1_1.svg \
	t1_2.svg \
	t1_3.svg \
	t1_4.svg \
	t1_5.svg \
	t1_6.svg \
	fig7.svg \

all: $(SVGS)

pa_vect_inside_sect.svg: src/pa_vect_inside_sect.rp
	pcb-rnd -x svg --outfile $@ src/pa_vect_inside_sect.rp

t1_1.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_1.rp

t1_2.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_2.rp

t1_3.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_3.rp

t1_4.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_4.rp

t1_5.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_5.rp

t1_6.svg:
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_6.rp

fig7.svg:
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig7.rp
