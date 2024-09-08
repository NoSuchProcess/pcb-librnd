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

RND_INLINE void pb2_5_cleanup(pb2_ctx_t *ctx)
{
	pb2_face_t *f, *next;

	/* cheap free of all destroyed faces */
	for(f = gdl_first(&ctx->faces); f != NULL; f = next) {
		next = gdl_next(&ctx->faces, f);
		if (f->destroy) {
			gdl_remove(&ctx->faces, f, link);
			free(f);
		}
	}
}

RND_INLINE void pb2_5_face_remap(pb2_ctx_t *ctx)
{
	pb2_curve_t *c;

	pb2_5_cleanup(ctx);
	pb2_2_face_map(ctx);
	pb2_3_face_polarity(ctx);


	for(c = gdl_first(&ctx->curves); c != NULL; c = gdl_next(&ctx->curves, c))
		if (!c->pruned)
			pb2_4_curve_other_face(ctx, c);
}
