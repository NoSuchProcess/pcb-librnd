/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2015 Tibor 'Igor2' Palinkas
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
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "rats_patch.h"
#include "genht/htsp.h"
#include "genht/hash.h"

#include "config.h"
#include "actions.h"
#include "data.h"
#include "error.h"
#include "copy.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "safe_fs.h"

static void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t * n, int do_free);

const char *pcb_netlist_names[PCB_NUM_NETLISTS] = {
	"input",
	"edited"
};

void pcb_ratspatch_append(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	pcb_ratspatch_line_t *n;

	n = malloc(sizeof(pcb_ratspatch_line_t));

	n->op = op;
	n->id = pcb_strdup(id);
	n->arg1.net_name = pcb_strdup(a1);
	if (a2 != NULL)
		n->arg2.attrib_val = pcb_strdup(a2);
	else
		n->arg2.attrib_val = NULL;

	/* link in */
	n->prev = pcb->NetlistPatchLast;
	if (pcb->NetlistPatches != NULL) {
		pcb->NetlistPatchLast->next = n;
		pcb->NetlistPatchLast = n;
	}
	else
		pcb->NetlistPatchLast = pcb->NetlistPatches = n;
	n->next = NULL;
}

static void rats_patch_free_fields(pcb_ratspatch_line_t *n)
{
	if (n->id != NULL)
		free(n->id);
	if (n->arg1.net_name != NULL)
		free(n->arg1.net_name);
	if (n->arg2.attrib_val != NULL)
		free(n->arg2.attrib_val);
}

void pcb_ratspatch_destroy(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n, *next;

	for(n = pcb->NetlistPatches; n != NULL; n = next) {
		next = n->next;
		rats_patch_free_fields(n);
		free(n);
	}
}

void pcb_ratspatch_append_optimize(pcb_board_t *pcb, pcb_rats_patch_op_t op, const char *id, const char *a1, const char *a2)
{
	pcb_rats_patch_op_t seek_op;
	pcb_ratspatch_line_t *n;

	switch (op) {
	case RATP_ADD_CONN:
		seek_op = RATP_DEL_CONN;
		break;
	case RATP_DEL_CONN:
		seek_op = RATP_ADD_CONN;
		break;
	case RATP_CHANGE_ATTRIB:
		seek_op = RATP_CHANGE_ATTRIB;
		break;
	}

	/* keep the list clean: remove the last operation that becomes obsolete by the new one */
	for (n = pcb->NetlistPatchLast; n != NULL; n = n->prev) {
		if ((n->op == seek_op) && (strcmp(n->id, id) == 0)) {
			switch (op) {
			case RATP_CHANGE_ATTRIB:
				if (strcmp(n->arg1.attrib_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (strcmp(n->arg1.net_name, a1) != 0)
					break;
				rats_patch_remove(pcb, n, 1);
				goto quit;
			}
		}
	}

quit:;
	pcb_ratspatch_append(pcb, op, id, a1, a2);
}

/* Unlink n from the list; if do_free is non-zero, also free fields and n */
static void rats_patch_remove(pcb_board_t *pcb, pcb_ratspatch_line_t * n, int do_free)
{
	/* if we are the first or last... */
	if (n == pcb->NetlistPatches)
		pcb->NetlistPatches = n->next;
	if (n == pcb->NetlistPatchLast)
		pcb->NetlistPatchLast = n->prev;

	/* extra ifs, just in case n is already unlinked */
	if (n->prev != NULL)
		n->prev->next = n->next;
	if (n->next != NULL)
		n->next->prev = n->prev;

	if (do_free) {
		rats_patch_free_fields(n);
		free(n);
	}
}

static void netlist_free(pcb_lib_t *dst)
{
	int n, p;

	if (dst->Menu == NULL)
		return;

	for (n = 0; n < dst->MenuN; n++) {
		pcb_lib_menu_t *dmenu = &dst->Menu[n];
		free(dmenu->Name);
		for (p = 0; p < dmenu->EntryN; p++) {
			pcb_lib_entry_t *dentry = &dmenu->Entry[p];
			free((char*)dentry->ListEntry);
		}
		free(dmenu->Entry);
	}
	free(dst->Menu);
	dst->Menu = NULL;
}

static void netlist_copy(pcb_lib_t *dst, pcb_lib_t *src)
{
	int n, p;
	dst->MenuMax = dst->MenuN = src->MenuN;
	if (src->MenuN != 0) {
		dst->Menu = calloc(sizeof(pcb_lib_menu_t), dst->MenuN);
		for (n = 0; n < src->MenuN; n++) {
			pcb_lib_menu_t *smenu = &src->Menu[n];
			pcb_lib_menu_t *dmenu = &dst->Menu[n];
/*		fprintf(stderr, "Net %d name='%s': ", n, smenu->Name+2);*/
			dmenu->Name = pcb_strdup(smenu->Name);
			dmenu->EntryMax = dmenu->EntryN = smenu->EntryN;
			dmenu->Entry = calloc(sizeof(pcb_lib_entry_t), dmenu->EntryN);
			dmenu->flag = smenu->flag;
			dmenu->internal = smenu->internal;
			for (p = 0; p < smenu->EntryN; p++) {
			pcb_lib_entry_t *sentry = &smenu->Entry[p];
				pcb_lib_entry_t *dentry = &dmenu->Entry[p];
				dentry->ListEntry = pcb_strdup(sentry->ListEntry);
				dentry->ListEntry_dontfree = 0;
/*			fprintf (stderr, " '%s'/%p", dentry->ListEntry, dentry->ListEntry);*/
			}
/*		fprintf(stderr, "\n");*/
		}
	}
	else
		dst->Menu = NULL;
}


int rats_patch_apply_conn(pcb_board_t *pcb, pcb_ratspatch_line_t * patch, int del)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[PCB_NETLIST_EDITED].MenuN; n++) {
		pcb_lib_menu_t *menu = &pcb->NetlistLib[PCB_NETLIST_EDITED].Menu[n];
		if (strcmp(menu->Name + 2, patch->arg1.net_name) == 0) {
			int p;
			for (p = 0; p < menu->EntryN; p++) {
				pcb_lib_entry_t *entry = &menu->Entry[p];
/*				fprintf (stderr, "C'%s'/%p '%s'/%p\n", entry->ListEntry, entry->ListEntry, patch->id, patch->id);*/
				if (strcmp(entry->ListEntry, patch->id) == 0) {
					if (del) {
						/* want to delete and it's on the list */
						memmove(&menu->Entry[p], &menu->Entry[p + 1], (menu->EntryN - p - 1) * sizeof(pcb_lib_entry_t));
						menu->EntryN--;
						return 0;
					}
					/* want to add, and pin is on the list -> already added */
					return 1;
				}
			}
			/* If we got here, pin is not on the list */
			if (del)
				return 1;

			/* Wanted to add, let's add it */
			pcb_lib_conn_new(menu, (char *) patch->id);
			return 0;
		}
	}

	/* couldn't find the net: create it */
	{
		pcb_lib_menu_t *net = NULL;
		net = pcb_lib_net_new(&pcb->NetlistLib[PCB_NETLIST_EDITED], patch->arg1.net_name, NULL);
		if (net == NULL)
			return 1;
		pcb_lib_conn_new(net, (char *) patch->id);
	}
	return 0;
}


int pcb_ratspatch_apply(pcb_board_t *pcb, pcb_ratspatch_line_t * patch)
{
	switch (patch->op) {
	case RATP_ADD_CONN:
		return rats_patch_apply_conn(pcb, patch, 0);
	case RATP_DEL_CONN:
		return rats_patch_apply_conn(pcb, patch, 1);
	case RATP_CHANGE_ATTRIB:
TODO("just check wheter it is still valid")
		break;
	}
	return 0;
}

void pcb_ratspatch_make_edited(pcb_board_t *pcb)
{
	pcb_ratspatch_line_t *n;

	netlist_free(&(pcb->NetlistLib[PCB_NETLIST_EDITED]));
	netlist_copy(&(pcb->NetlistLib[PCB_NETLIST_EDITED]), &(pcb->NetlistLib[PCB_NETLIST_INPUT]));
	for (n = pcb->NetlistPatches; n != NULL; n = n->next)
		pcb_ratspatch_apply(pcb, n);
}

static pcb_lib_menu_t *rats_patch_find_net(pcb_board_t *pcb, const char *netname, int listidx)
{
	int n;

	for (n = 0; n < pcb->NetlistLib[listidx].MenuN; n++) {
		pcb_lib_menu_t *menu = &pcb->NetlistLib[listidx].Menu[n];
		if (strcmp(menu->Name + 2, netname) == 0)
			return menu;
	}
	return NULL;
}

int pcb_rats_patch_export(pcb_board_t *pcb, pcb_ratspatch_line_t *pat, pcb_bool need_info_lines, void (*cb)(void *ctx, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val), void *ctx)
{
	pcb_ratspatch_line_t *n;

	if (need_info_lines) {
		htsp_t *seen;
		seen = htsp_alloc(strhash, strkeyeq);

		/* have to print net_info lines */
		for (n = pat; n != NULL; n = n->next) {
			switch (n->op) {
			case RATP_ADD_CONN:
			case RATP_DEL_CONN:
				if (htsp_get(seen, n->arg1.net_name) == NULL) {
					pcb_lib_menu_t *net;
					int p;

					net = rats_patch_find_net(pcb, n->arg1.net_name, PCB_NETLIST_INPUT);
/*					printf("net: '%s' %p\n", n->arg1.net_name, (void *)net);*/
					if (net != NULL) {
						htsp_set(seen, n->arg1.net_name, net);
						cb(ctx, PCB_RPE_INFO_BEGIN, n->arg1.net_name, NULL, NULL);
						for (p = 0; p < net->EntryN; p++) {
							pcb_lib_entry_t *entry = &net->Entry[p];
							cb(ctx, PCB_RPE_INFO_TERMINAL, n->arg1.net_name, NULL, entry->ListEntry);
						}
						cb(ctx, PCB_RPE_INFO_END, n->arg1.net_name, NULL, NULL);

					}
				}
			case RATP_CHANGE_ATTRIB:
				break;
			}
		}
		htsp_free(seen);
	}

	/* action lines */
	for (n = pat; n != NULL; n = n->next) {
		switch (n->op) {
		case RATP_ADD_CONN:
			cb(ctx, PCB_RPE_CONN_ADD, n->arg1.net_name, NULL, n->id);
			break;
		case RATP_DEL_CONN:
			cb(ctx, PCB_RPE_CONN_DEL, n->arg1.net_name, NULL, n->id);
			break;
		case RATP_CHANGE_ATTRIB:
			cb(ctx, PCB_RPE_ATTR_CHG, n->id, n->arg1.attrib_name, n->arg2.attrib_val);
			break;
		}
	}
	return 0;
}

typedef struct {
	FILE *f;
	const char *q, *po, *pc, *line_prefix;
} fexport_t;

static void fexport_cb(void *ctx_, pcb_rats_patch_export_ev_t ev, const char *netn, const char *key, const char *val)
{
	fexport_t *ctx = ctx_;
	switch(ev) {
		case PCB_RPE_INFO_BEGIN:     fprintf(ctx->f, "%snet_info%s%s%s%s", ctx->line_prefix, ctx->po, ctx->q, netn, ctx->q); break;
		case PCB_RPE_INFO_TERMINAL:  fprintf(ctx->f, " %s%s%s", ctx->q, val, ctx->q); break;
		case PCB_RPE_INFO_END:       fprintf(ctx->f, "%s\n", ctx->pc); break;
		case PCB_RPE_CONN_ADD:       fprintf(ctx->f, "%sadd_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_CONN_DEL:       fprintf(ctx->f, "%sdel_conn%s%s%s%s %s%s%s%s\n", ctx->line_prefix, ctx->po, ctx->q, val, ctx->q, ctx->q, netn, ctx->q, ctx->pc); break;
		case PCB_RPE_ATTR_CHG:
			fprintf(ctx->f, "%schange_attrib%s%s%s%s %s%s%s %s%s%s%s\n",
				ctx->line_prefix, ctx->po,
				ctx->q, netn, ctx->q,
				ctx->q, key, ctx->q,
				ctx->q, val, ctx->q,
				ctx->pc);
	}
}

int pcb_ratspatch_fexport(pcb_board_t *pcb, FILE *f, int fmt_pcb)
{
	fexport_t ctx;
	if (fmt_pcb) {
		ctx.q = "\"";
		ctx.po = "(";
		ctx.pc = ")";
		ctx.line_prefix = "\t";
	}
	else {
		ctx.q = "";
		ctx.po = " ";
		ctx.pc = "";
		ctx.line_prefix = "";
	}
	ctx.f = f;
	return pcb_rats_patch_export(pcb, pcb->NetlistPatches, !fmt_pcb, fexport_cb, &ctx);
}


static const char pcb_acts_ReplaceFootprint[] = "ReplaceFootprint()\n";
static const char pcb_acth_ReplaceFootprint[] = "Replace the footprint of the selected components with the footprint specified.";

static fgw_error_t pcb_act_ReplaceFootprint(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *fpname = NULL;
	int found = 0, len, changed = 0;
	pcb_subc_t *news, *placed;

	/* check if we have elements selected and quit if not */
	PCB_SUBC_LOOP(PCB->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) && (subc->refdes != NULL)) {
			found = 1;
			break;
		}
	}
	PCB_END_LOOP;

	if (!(found)) {
		pcb_message(PCB_MSG_ERROR, "ReplaceFootprint works on selected subcircuits with refdes, please select subcircuits first!\n");
		PCB_ACT_IRES(1);
		return 0;
	}

	/* fetch the name of the new footprint */
	PCB_ACT_MAY_CONVARG(1, FGW_STR, ReplaceFootprint, fpname = pcb_strdup(argv[1].val.str));
	if (fpname == NULL) {
		fpname = pcb_hid_prompt_for("Footprint name to use for replacement:", "", "Footprint");
		if (fpname == NULL) {
			pcb_message(PCB_MSG_ERROR, "No footprint name supplied\n");
			PCB_ACT_IRES(1);
			return 0;
		}
	}
	else
		fpname = pcb_strdup(fpname);

	/* check if the footprint is available */
	pcb_buffer_load_footprint(&pcb_buffers[PCB_MAX_BUFFER-1], fpname, NULL);
	len = pcb_subclist_length(&pcb_buffers[PCB_MAX_BUFFER-1].Data->subc);
	if (len == 0) {
		free(fpname);
		pcb_message(PCB_MSG_ERROR, "Footprint %s contains no subcircuits", fpname);
		PCB_ACT_IRES(1);
		return 0;
	}
	if (len > 1) {
		free(fpname);
		pcb_message(PCB_MSG_ERROR, "Footprint %s contains multiple subcircuits", fpname);
		PCB_ACT_IRES(1);
		return 0;
	}
	news = pcb_subclist_first(&pcb_buffers[PCB_MAX_BUFFER-1].Data->subc);

	/* action: replace selected elements */
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_coord_t ox, oy;
		double rot = 0;
		long int target_id;

		if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc) || (subc->refdes == NULL))
			continue;

		if (pcb_subc_get_origin(subc, &ox, &oy) != 0) {
			ox = (subc->BoundingBox.X1 + subc->BoundingBox.X2) / 2;
			oy = (subc->BoundingBox.Y1 + subc->BoundingBox.Y2) / 2;
		}
		pcb_subc_get_rotation(subc, &rot);


		placed = pcb_subc_dup_at(PCB, PCB->Data, news, ox, oy, 0);

		pcb_ratspatch_append_optimize(PCB, RATP_CHANGE_ATTRIB, subc->refdes, "footprint", fpname);
		{ /* copy attributes */
			int n;
			pcb_attribute_list_t *dst = &placed->Attributes, *src = &subc->Attributes;
			for (n = 0; n < src->Number; n++)
				if (strcmp(src->List[n].name, "footprint") != 0)
					pcb_attribute_put(dst, src->List[n].name, src->List[n].value);
		}
		target_id = subc->ID;
		pcb_subc_remove(subc);

		pcb_obj_id_del(PCB->Data, placed);
		placed->ID = target_id;
		pcb_obj_id_reg(PCB->Data, placed);

		if (rot != 0)
			pcb_subc_rotate(placed, ox, oy, cos(rot / PCB_RAD_TO_DEG), sin(rot / PCB_RAD_TO_DEG), rot);
		changed = 1;
	}
	PCB_END_LOOP;

	if (changed)
		pcb_gui->invalidate_all();
	free(fpname);
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_SavePatch[] = "SavePatch(filename)";
static const char pcb_acth_SavePatch[] = "Save netlist patch for back annotation.";
static fgw_error_t pcb_act_SavePatch(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fn = NULL;
	FILE *f;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, SavePatch, fn = argv[1].val.str);

	if (fn == NULL) {
		char *default_file;

		if (PCB->Filename != NULL) {
			char *end;
			int len;
			len = strlen(PCB->Filename);
			default_file = malloc(len + 8);
			memcpy(default_file, PCB->Filename, len + 1);
			end = strrchr(default_file, '.');
			if ((end == NULL) || (pcb_strcasecmp(end, ".pcb") != 0))
				end = default_file + len;
			strcpy(end, ".bap");
		}
		else
			default_file = pcb_strdup("unnamed.bap");

		fn = pcb_gui->fileselect("Save netlist patch as ...",
			"Choose a file to save netlist patch to\n"
			"for back annotation\n", default_file, ".bap", "patch", 0);

		free(default_file);
	}

	f = pcb_fopen(fn, "w");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open netlist patch file %s for writing\n", fn);
		PCB_ACT_IRES(-1);
		return 0;
	}
	pcb_ratspatch_fexport(PCB, f, 0);
	fclose(f);

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t rats_patch_action_list[] = {
	{"ReplaceFootprint", pcb_act_ReplaceFootprint, pcb_acth_ReplaceFootprint, pcb_acts_ReplaceFootprint},
	{"SavePatch", pcb_act_SavePatch, pcb_acth_SavePatch, pcb_acts_SavePatch}
};

PCB_REGISTER_ACTIONS(rats_patch_action_list, NULL)
