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
 */


RND_INLINE int big_bool_ppl_(rnd_polyarea_t *pa, rnd_pline_t *pl, int is_hole)
{
	rnd_vnode_t *v = pl->head;
	do {
		if (v->flg.rounded) {
		}
	} while((v = v->next) != pl->head);

	return 0;
}

/* Does an iteration of postprocessing; returns whether pa changed (0 or 1) */
RND_INLINE int big_bool_ppa_(rnd_polyarea_t *pa)
{
	rnd_polyarea_t *pn = pa;
	do {
		rnd_pline_t *pl = pn->contours;
		if (pl != NULL) {
			if (big_bool_ppl_(pn, pl, 0))
				return 1;

			for(pl = pa->contours->next; pl != NULL; pl = pl->next)
				if (big_bool_ppl_(pn, pl, 0))
					return 1;
		}
	} while ((pn = pn->f) != pa);
	return 0;
}

void pa_big_bool_postproc(rnd_polyarea_t *pa)
{
	/* keep iterating as long as there are things to be fixed */
	while(big_bool_ppa_(pa)) ;
}

