/*
 *                            COPYRIGHT
 *
 *  libpolybool, 2D polygon bool operations
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
 *
 *  (Supported by NLnet NGI0 Entrust in 2024)
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
 */

#include <stdio.h>
#include <librnd/rnd_config.h>
#include <librnd/core/misc_util.h>
#include <math.h>
#include "pa_math.c"
#include "rtree.h"
#include "polyarea.h"
#include "pb2.h"

/* These work only because rnd_vector_t is an array so it is packed. */
#define Vcpy2(dst, src)   memcpy((dst), (src), sizeof(rnd_vector_t))
#define Vequ2(a,b)       (memcmp((a),   (b),   sizeof(rnd_vector_t)) == 0)
#define Vsub2(r, a, b) \
	do { \
		(r)[0] = (a)[0] - (b)[0]; \
		(r)[1] = (a)[1] - (b)[1]; \
	} while(0)

#include "pb2_debug_txt.c"
#include "pb2_debug_svg.c"
#include "pb2_admin.c"
#include "pb2_geo.c"

#include "pb2_1.c"
#include "pb2_2.c"
#include "pb2_3.c"
#include "pb2_4.c"
#include "pb2_5.c"
#include "pb2_6.c"
#include "pb2_7.c"
#include "pb2_8.c"

int pb2_debug_dump_steps = 0, pb2_debug_draw_steps = 0;

int pb2_exec(pb2_ctx_t *ctx, rnd_polyarea_t **res)
{
	int changed;

	pb2_1_to_topo(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 1 topo:\n"); pb2_dump(ctx, PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH); }
	if (pb2_debug_draw_steps) { pb2_draw(ctx, "step1.svg", PB2_DRAW_INPUT_POLY | PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH); }

	pb2_2_face_map(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 2 map:\n"); pb2_dump(ctx, PB2_DUMP_FACES); }

	pb2_3_face_polarity(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 3 polarity:\n"); pb2_dump(ctx, PB2_DUMP_FACES); }
	if (pb2_debug_draw_steps) { pb2_draw(ctx, "step3.svg", PB2_DRAW_INPUT_POLY | PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH | PB2_DUMP_FACES); }

	changed = pb2_4_prune_curves(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 4 prune curves (chg=%d):\n", changed); pb2_dump(ctx, PB2_DUMP_CURVES | PB2_DUMP_FACES); }

	if (changed)
		pb2_5_face_remap(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 5 output faces:\n"); pb2_dump(ctx, PB2_DUMP_CURVES | PB2_DUMP_FACES); }
	if (pb2_debug_draw_steps) { pb2_draw(ctx, "step5.svg", PB2_DRAW_INPUT_POLY | PB2_DUMP_SEGS | PB2_DUMP_CURVES | PB2_DUMP_CURVE_GRAPH | PB2_DUMP_FACES); }

	pb2_6_polarity(ctx);
	if (pb2_debug_dump_steps) { rnd_trace("step 6 output face tree:\n"); pb2_dump(ctx, PB2_DUMP_FACE_TREE); }

	pb2_7_output(ctx, res);
	pb2_8_cleanup(ctx);

	return 0;
}
