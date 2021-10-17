/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2021 Tibor 'Igor2' Palinkas
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

/* Private utility functions for reading and writing fsd persistence files 
   and dealing with file system related low levels */

static int fsd_shcut_path_setup(fsd_ctx_t *ctx, gds_t *path, int per_dlg, int do_mkdir)
{
	if (rnd_conf.rc.path.home == NULL)
		return -1;

	gds_append_str(path, rnd_conf.rc.path.home);
	gds_append(path, '/');

	gds_append_str(path, rnd_app.dot_dir);
	gds_append_str(path, "/fsd");
	if (do_mkdir && !rnd_is_dir(ctx->hidlib, path->array))
		rnd_mkdir(ctx->hidlib, path->array, 0750);

	gds_append(path, '/');
	if (per_dlg)
		gds_append_str(path, ctx->history_tag);

	return 0;
}

static void fsd_shcut_load_strip(char *line)
{
	char *end = line + strlen(line) - 1;
	while((end >= line) && ((*end == '\n') || (*end == '\r'))) { *end = '\0'; end--; }
}

static void fsd_shcut_load_file(fsd_ctx_t *ctx, rnd_hid_attribute_t *attr, rnd_hid_row_t *rparent, gds_t *path, const char *suffix)
{
	int saved = path->used;
	FILE *f;

	gds_append_str(path, suffix);

	f = rnd_fopen(ctx->hidlib, path->array, "r");
	if (f != NULL) {
		char line[RND_PATH_MAX+8], *cell[2];
		cell[1] = NULL;
		while(fgets(line, sizeof(line), f) != NULL) {
			fsd_shcut_load_strip(line);
			if (*line == '\0')
				continue;
			cell[0] = rnd_strdup(line);
			rnd_dad_tree_append_under(attr, rparent, cell);
		}
		fclose(f);
	}

	path->used = saved;
}

static int fsd_shcut_change_file(fsd_ctx_t *ctx, int per_dlg, const char *suffix, const char *add_entry, const char *del_entry, int limit)
{
	gds_t path = {0};
	FILE *fi, *fo;
	char *target;
	int res = 0;

	if (fsd_shcut_path_setup(ctx, &path, per_dlg, 1) != 0) {
		rnd_message(RND_MSG_ERROR, "Failed to open/create fsd/ in application $HOME dotdir\n");
		return;
	}

	gds_append_str(&path, suffix);
	target = rnd_strdup(path.array);
	fi = rnd_fopen(ctx->hidlib, target, "r");
	gds_append_str(&path, ".tmp");
	fo = rnd_fopen(ctx->hidlib, path.array, "w");
	if (fo == NULL) {
		rnd_message(RND_MSG_ERROR, "Failed to open '%s' for write\n", path.array);
		if (fi != NULL)
			fclose(fi);
		goto out;
	}

	if (fi != NULL) {
		char line[RND_PATH_MAX+8];
		int lines = 0;

		/* count lines to see if we need to remove one */
		while(fgets(line, sizeof(line), fi) != NULL) {
			fsd_shcut_load_strip(line);
			if (*line == '\0')
				continue;
			if ((add_entry != NULL) && (strcmp(line, add_entry) == 0))
				lines--;
			lines++;
		}

		rewind(fi);
		if ((limit > 0) && (lines >= limit)) { /* read one non-empty line */
			int nremove = lines - limit + 1;
			while(fgets(line, sizeof(line), fi) != NULL) {
				fsd_shcut_load_strip(line);
				if (*line != '\0') {
					nremove--;
					if (nremove <= 0)
						break;
				}
			}
		}

		/* copy existing lines (except empty lines and add/del entries) */
		while(fgets(line, sizeof(line), fi) != NULL) {
			fsd_shcut_load_strip(line);
			if (*line == '\0') continue;
			if ((add_entry != NULL) && (strcmp(line, add_entry) == 0)) continue;
			if ((del_entry != NULL) && (strcmp(line, del_entry) == 0)) { res = 1; continue; }
			fprintf(fo, "%s\n", line); 
		}
	}

	/* append new line */
	if (add_entry != NULL) {
		fprintf(fo, "%s\n", add_entry);
		res = 1;
	}

	/* safe overwrite with mv */
	fclose(fo);
	if (fi != NULL)
		fclose(fi);
	rnd_rename(ctx->hidlib, path.array, target);

	out:;
	free(target);
	gds_uninit(&path);
	return res;
}

/* Appends entry to an fsd persistence file and returns 1 if the file changed. */
static int fsd_shcut_append_to_file(fsd_ctx_t *ctx, int per_dlg, const char *suffix, const char *entry, int limit)
{
	return fsd_shcut_change_file(ctx, per_dlg, suffix, entry, NULL, limit);
}

static int fsd_shcut_del_from_file(fsd_ctx_t *ctx, int per_dlg, const char *suffix, const char *entry)
{
	return fsd_shcut_change_file(ctx, per_dlg, suffix, NULL, entry, 0);
}


char *fsd_io_rsep(char *path)
{
#ifdef __WIN32__
	char *s = path + strlen(path) - 1;
	while(s >= path) {
		if ((*s == '/') || (*s == '\\'))
			return s;
		s--;
	}
	return NULL;
#else
	return strrchr(path, '/');
#endif
}
