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

RND_INLINE void pb2_8_cleanup(pb2_ctx_t *ctx)
{
	pb2_curve_t *c, *cn;
	pb2_face_t *f, *fn;
	pb2_cgnode_t *n, *nn;
	pb2_seg_t *s, *sn;

	rnd_rtree_uninit(&ctx->seg_tree);

	/* free faces */
	for(f = gdl_first(&ctx->faces); f != NULL; f = fn) {
		fn = gdl_next(&ctx->faces, f);
		free(f);
	}

	/* free curves */
	for(c = gdl_first(&ctx->curves); c != NULL; c = cn) {
		cn = gdl_next(&ctx->curves, c);
		free(c);
	}

	/* free nodes */
	for(n = gdl_first(&ctx->cgnodes); n != NULL; n = nn) {
		nn = gdl_next(&ctx->cgnodes, n);
		free(n);
	}

	/* free segs */
	for(s = ctx->all_segs; s != NULL; s = sn) {
		sn = s->next_all;
		free(s);
	}

	/* Note: outs are not free'd, they are allocated as part of nodes */

	vtp0_uninit(&ctx->outtmp);
}
