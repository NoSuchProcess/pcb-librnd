/*
 *                            COPYRIGHT
 *
 *  librnd, modular 2D CAD framework
 *  Copyright (C) 2022 Tibor 'Igor2' Palinkas
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

#ifndef RND_PROJECT_H
#define RND_PROJECT_H

#include <liblihata/lihata.h>
#include <liblihata/dom.h>
#include <genvector/vtp0.h>
#include <genht/htsp.h>
#include <librnd/core/global_typedefs.h>

/* Generic part of a project struct; should be the first field (called "hdr")
   of an app's project struct */
struct rnd_project_s {
	char *loadname;  /* file name as specified by the user */
	char *fullpath;  /* loadname resolved with realpath() - the actual full path file name on the file system */
	char *prjdir;    /* real path to the directory that hosts the project file */
	vtp0_t designs;  /* list of (rnd_design_t *) that are loaded for this project */
	lht_doc_t *root; /* shared conf doc for all designs; rnd_conf_main_root[RND_CFR_PROJECT] will point to this */
};

/* all open projects; key is real file name of project file, value is (rnd_project_t *) */
htsp_t rnd_projects;

/* Free fields of project; doesn't free project itself. Assumes items on
   project->designs are free'd by the caller. Any design still on the
   priject's designs list will be reset to have no project (dsg->project == NULL) */
void rnd_project_uninit(rnd_project_t *project);

/* Append dsg at the end of prj->designs, if dsg is not already in a project.
   Returns 0 on success. */
int rnd_project_append_design(rnd_project_t *prj, rnd_design_t *dsg);

/* Remove dsg from prj's ->designs and return number of removals (should be
   0 or 1 normally) */
int rnd_project_remove_design(rnd_project_t *prj, rnd_design_t *dsg);

/* Recalculate the ->hdr.filename and ->hdr.prjdir of prj using its
   ->hdr.loadname field */
int rnd_project_update_filename(rnd_project_t *prj);


/* These are usually called from hidlib_(un)init* */
void rnd_projects_init(void);
void rnd_projects_uninit(void);



#endif
