SVGS= \
	pa_vect_inside_sect.svg \
	t1_1.svg \
	t1_2.svg \
	t1_3.svg \
	t1_4.svg \
	t1_5.svg \
	t1_6.svg \
	fig1.svg \
	fig2.svg \
	fig3.svg \
	fig4.svg \
	fig5.svg \
	fig6.svg \
	fig7.svg \
	selfi1.svg \
	selfi2.svg \
	gixedb.svg \

all: $(SVGS)

pa_vect_inside_sect.svg: src/pa_vect_inside_sect.rp
	pcb-rnd -x svg --outfile $@ src/pa_vect_inside_sect.rp

t1_1.svg: src/t1_1.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_1.rp

t1_2.svg: src/t1_2.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_2.rp

t1_3.svg: src/t1_3.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_3.rp

t1_4.svg: src/t1_4.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_4.rp

t1_5.svg: src/t1_5.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_5.rp

t1_6.svg: src/t1_6.rp
	pcb-rnd -x svg --opacity 66 --cam $@=top-doc src/t1_6.rp

fig1.svg: src/fig1.rp
	pcb-rnd -x svg --opacity 75 --cam $@=top-doc src/fig1.rp

fig2.svg: src/fig2.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig2.rp

fig3.svg: src/fig3.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig3.rp

fig4.svg: src/fig4.rp
	pcb-rnd -x svg --opacity 75 --cam $@=top-doc src/fig4.rp

fig5.svg: src/fig5.rp
	pcb-rnd -x svg --opacity 75 --cam $@=top-doc src/fig5.rp

fig6.svg: src/fig6.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig6.rp

fig7.svg: src/fig7.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig7.rp

selfi1.svg: src/selfi1.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-copper src/selfi1.rp

selfi2.svg: src/selfi2.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-copper src/selfi2.rp

gixedb.svg: src/gixedb.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/gixedb.rp
