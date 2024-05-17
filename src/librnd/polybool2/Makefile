TRUNK=../../../trunk

#PROF=-pg
PROF=-O0

CFLAGS = -Wall $(PROF) -g -I/usr/local/include/librnd4 -I/usr/include/librnd4 \
  -I/usr/local/include/librnd4/librnd/src_3rd -I/usr/include/librnd4/librnd/src_3rd

OBJS = pb2.o polyarea.o polygon1_gen.o rtree.o offset.o
STEPS = pb2_1.c pb2_2.c pb2_3.c pb2_4.c pb2_5.c pb2_6.c pb2_7.c pb2_8.c
INFRA = pb2_debug_txt.c pb2_debug_svg.c pb2_admin.c pb2_geo.c pb2_glue_pa.c

polybool2.a: $(OBJS)
	ar r polybool2.a $(OBJS)

polyarea.o: polyarea.c pa_*.c pa_*.h polyarea.h pb2_glue_pa.c

pb2.o: pb2.c pb2.h polyarea.h $(INFRA) $(STEPS)

polygon1_gen.o: polygon1_gen.c polyarea.h

rtree.o: rtree.c rtree.h

offset.o: offset.c offset.h
