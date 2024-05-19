/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2023)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.*
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *  This is a full rewrite of pcb-rnd's (and PCB's) polygon lib originally
 *  written by Harry Eaton in 2006, in turn building on "poly_Boolean: a
 *  polygon clip library" by Alexey Nikitin, Michael Leonov from 1997 and
 *  "nclip: a polygon clip library" Klamer Schutte from 1993.
 *
 *  English translation of the original paper the lib is largely based on:
 *  https://web.archive.org/web/20160418014630/http://www.complex-a5.ru/polyboolean/downloads/polybool_eng.pdf
 *
 */

#include "pb2.h"

#include "pb2_glue_pa.c"

int rnd_polybool_disable_autocheck = 0;
int rnd_polybool_dump_boolops = 0;

#ifndef NDEBUG
#include <librnd/core/safe_fs.h>
RND_INLINE void pa_polyarea_bool_dbg(rnd_polyarea_t *A, rnd_polyarea_t *B, int op)
{
	if (rnd_polybool_dump_boolops != 0) {
		char fn[256], *opname;
		FILE *f;

		switch (op) {
			case RND_PBO_XOR:    opname = "xor"; break;
			case RND_PBO_UNITE:  opname = "unite"; break;
			case RND_PBO_SUB:    opname = "sub"; break;
			case RND_PBO_ISECT:  opname = "isect"; break;
			default:             opname = "UNKNOWN"; break;
		}

		sprintf(fn, "pb_%08d.A.poly", rnd_polybool_dump_boolops);
		pa_dump_pa(A, fn);
		fn[12] = 'B';
		pa_dump_pa(B, fn);

		strcpy(fn+12, opname);
		f = rnd_fopen(NULL, fn, "w");
		fclose(f);

		rnd_polybool_dump_boolops++;
		rnd_trace("dumped %s\n", fn);
	}
}
#else
RND_INLINE void pa_polyarea_bool_dbg(rnd_polyarea_t *A, rnd_polyarea_t *B, int op) {}
#endif


static int rnd_polyarea_boolean_(rnd_polyarea_t *A, rnd_polyarea_t *B, rnd_polyarea_t **res, int op, rnd_bool preserve)
{
	pb2_ctx_t ctx = {0};
	int retval;

	pa_polyarea_bool_dbg(A, B, op);

	*res = NULL;
	rnd_rtree_init(&ctx.seg_tree);

	ctx.input_A = A;
	ctx.input_B = B;
	ctx.op = op;
	ctx.has_B = 1;

	pb2_pa_clear_overlaps(A);
	pb2_pa_clear_overlaps(B);
	pb2_pa_map_overlaps(A, B);

	pb2_pa_map_polyarea(&ctx, A, 'A', 0);
	pb2_pa_map_polyarea(&ctx, B, 'B', 0);


	retval = pb2_exec(&ctx, res);

	pb2_pa_apply_nonoverlaps(res, A, B, op, preserve);

	return retval;
}

int rnd_polyarea_boolean(const rnd_polyarea_t *A, const rnd_polyarea_t *B, rnd_polyarea_t **res, int op)
{
	return rnd_polyarea_boolean_((rnd_polyarea_t *)A, (rnd_polyarea_t *)B, res, op, 1);
}


rnd_cardinal_t rnd_polyarea_split_selfisc(rnd_polyarea_t **pa)
{
	rnd_polyarea_t *respa = NULL;
	int res;
	pb2_ctx_t ctx = {0};

	rnd_rtree_init(&ctx.seg_tree);

	ctx.input_A = *pa;
	ctx.op = RND_PBO_CANON;

	pb2_pa_map_polyarea(&ctx, *pa, 'A', 1);

	res = pb2_exec(&ctx, &respa);

	pa_polyarea_free_all(pa);
	*pa = respa;

	return res;
}



int rnd_polyarea_boolean_free_nochk(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	rnd_polyarea_t *a = a_, *b = b_;
	int code;

	*res = NULL;

	/* handle the case when either input is empty */
	if (a == NULL) {
		switch (op) {
			case RND_PBO_XOR:
			case RND_PBO_UNITE:
				*res = b_;
				return pa_err_ok;
			case RND_PBO_SUB:
			case RND_PBO_ISECT:
				if (b != NULL)
					pa_polyarea_free_all(&b);
				return pa_err_ok;
		}
	}
	if (b == NULL) {
		switch (op) {
			case RND_PBO_SUB:
			case RND_PBO_XOR:
			case RND_PBO_UNITE:
				*res = a_;
				return pa_err_ok;
			case RND_PBO_ISECT:
				if (a != NULL)
					pa_polyarea_free_all(&a);
				return pa_err_ok;
		}
	}

	code = rnd_polyarea_boolean_(a_, b_, res, op, 0);

	pa_polyarea_free_all(&a_);
	pa_polyarea_free_all(&b_);

	return code;
}

int rnd_polyarea_boolean_free(rnd_polyarea_t *a_, rnd_polyarea_t *b_, rnd_polyarea_t **res, int op)
{
	int code = rnd_polyarea_boolean_free_nochk(a_, b_, res, op);

	if ((code == 0) && (*res != NULL) && !rnd_polybool_disable_autocheck) {
		assert(rnd_poly_valid(*res));
	}

	return code;
}

int rnd_polyarea_and_subtract_free(rnd_polyarea_t *a, rnd_polyarea_t *b, rnd_polyarea_t **aandb, rnd_polyarea_t **aminusb)
{
	assert(!"rnd_polyarea_and_subtract_free() not implemented - use the new clip code");
	abort();
}
