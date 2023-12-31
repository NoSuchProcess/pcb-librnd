/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2020 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://www.repo.hu/projects/librnd
 *    lead developer: http://www.repo.hu/projects/librnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef RND_TOOL_H
#define RND_TOOL_H

#include <genvector/vtp0.h>

#include <librnd/core/global_typedefs.h>
#include <librnd/core/rnd_bool.h>

typedef int rnd_toolid_t;
#define RND_TOOLID_INVALID (-1)

typedef struct rnd_tool_cursor_s {
	const char *name;            /* if no custom graphics is provided, use a stock cursor by name */
	const unsigned char *pixel;  /* 32 bytes: 16*16 bitmap */
	const unsigned char *mask;   /* 32 bytes: 16*16 mask (1 means draw pixel) */
} rnd_tool_cursor_t;

#define RND_TOOL_CURSOR_NAMED(name)       { name, NULL, NULL }
#define RND_TOOL_CURSOR_XBM(pixel, mask)  { NULL, pixel, mask }

typedef enum rnd_tool_flags_e {
	RND_TLF_AUTO_TOOLBAR = 1      /* automatically insert in the toolbar if the menu file didn't do it */
} rnd_tool_flags_t;

typedef struct rnd_tool_s {
	const char *name;             /* textual name of the tool */
	const char *help;             /* help/tooltip that explains the feature */
	const char *cookie;           /* plugin cookie _pointer_ of the registrar (comparision is pointer based, not strcmp) */
	unsigned int priority;        /* lower values are higher priorities; escaping mode will try to select the highest prio tool */
	const char **icon;            /* XPM for the tool buttons */
	rnd_tool_cursor_t cursor;     /* name of the mouse cursor to switch to when the tool is activated */
	rnd_tool_flags_t flags;

	/* tool implementation */
	void     (*init)(void);
	void     (*uninit)(void);
	void     (*press)(rnd_design_t *hl);
	void     (*release)(rnd_design_t *hl);
	void     (*adjust_attached)(rnd_design_t *hl);
	void     (*draw_attached)(rnd_design_t *hl);
	rnd_bool (*undo_act)(rnd_design_t *hl);
	rnd_bool (*redo_act)(rnd_design_t *hl);
	void     (*escape)(rnd_design_t *hl);
	
	unsigned long user_flags;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2, spare_l3, spare_l4;
	void *spare_p1, *spare_p2, *spare_p3, *spare_p4;
	double spare_d1, spare_d2, spare_d3, spare_d4;
	rnd_coord_t spare_c1, spare_c2, spare_c3, spare_c4;
} rnd_tool_t;

extern vtp0_t rnd_tools;
extern rnd_toolid_t rnd_tool_prev_id;
extern rnd_toolid_t rnd_tool_next_id;
extern rnd_bool rnd_tool_is_saved;

/* (un)initialize the tool subsystem */
void rnd_tool_init(void);
void rnd_tool_uninit(void);

/* call this when the mode (tool) config node changes */
void rnd_tool_chg_mode(rnd_design_t *hl);

/* Insert a new tool in rnd_tools; returns -1 on failure */
rnd_toolid_t rnd_tool_reg(rnd_tool_t *tool, const char *cookie);

/* Unregister all tools that has matching cookie */
void rnd_tool_unreg_by_cookie(const char *cookie);

/* Return the ID of a tool by name; returns -1 on error */
rnd_toolid_t rnd_tool_lookup(const char *name);


/* Select a tool by name, id or pick the highest prio tool; return 0 on success */
int rnd_tool_select_by_name(rnd_design_t *design, const char *name);
int rnd_tool_select_by_id(rnd_design_t *design, rnd_toolid_t id);
int rnd_tool_select_highest(rnd_design_t *design);

int rnd_tool_save(rnd_design_t *design);
int rnd_tool_restore(rnd_design_t *design);

/* Called after GUI_INIT; registers all mouse cursors in the GUI */
void rnd_tool_gui_init(void);


/**** Tool function wrappers; calling these will operate on the current tool 
      as defined in rnd_conf.editor.mode ****/

void rnd_tool_press(rnd_design_t *design);
void rnd_tool_release(rnd_design_t *design);
void rnd_tool_adjust_attached(rnd_design_t *hl);
void rnd_tool_draw_attached(rnd_design_t *hl);
rnd_bool rnd_tool_undo_act(rnd_design_t *hl);
rnd_bool rnd_tool_redo_act(rnd_design_t *hl);


/* fake a click */
void rnd_tool_do_press(rnd_design_t *design);

/**** Low level, for internal use ****/

/* Get the tool pointer of a tool by id */
RND_INLINE const rnd_tool_t *rnd_tool_get(long id)
{
	rnd_tool_t **tp = (rnd_tool_t **)vtp0_get(&rnd_tools, id, 0);
	if (tp == NULL) return NULL;
	return *tp;
}

#endif
