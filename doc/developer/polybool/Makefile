SVGS= \
	pa_vect_inside_sect.svg \
	t1_1.svg \
	t1_2.svg \
	t1_3.svg \
	t1_4.svg \
	t1_5.svg \
	t1_6.svg \
	fig2.svg \
	fig3.svg \
	fig4.svg \
	fig5.svg \
	fig7.svg \

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

fig2.svg: src/fig2.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig2.rp

fig3.svg: src/fig3.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig3.rp

fig4.svg: src/fig4.rp
	pcb-rnd -x svg --opacity 75 --cam $@=top-doc src/fig4.rp

fig5.svg: src/fig5.rp
	pcb-rnd -x svg --opacity 75 --cam $@=top-doc src/fig5.rp

fig7.svg: src/fig7.rp
	pcb-rnd -x svg --opacity 100 --cam $@=top-doc src/fig7.rp
