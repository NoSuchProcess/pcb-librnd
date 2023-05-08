/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2023 Tibor 'Igor2' Palinkas
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

#include <librnd/rnd_config.h>
#include <librnd/core/actions.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/compat_lrealpath.h>
#include <librnd/core/error.h>


static const char rnd_acts_SafeFsSystem[] = "SafeFsSystem(cmdline)";
static const char rnd_acth_SafeFsSystem[] = "Runs cmdline with a shell using librnd safe_fs. Return value is the same integer as system()'s";
static fgw_error_t rnd_act_SafeFsSystem(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *cmd;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsSystem, cmd = argv[1].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_system(RND_ACT_DESIGN, cmd);
	return 0;
}

static const char rnd_acts_SafeFsRemove[] = "SafeFsRemove(path)";
static const char rnd_acth_SafeFsRemove[] = "Remove an object from the file system. Return value is the same as remove(3)'s";
static fgw_error_t rnd_act_SafeFsRemove(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsRemove, path = argv[1].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_remove(RND_ACT_DESIGN, path);
	return 0;
}

static const char rnd_acts_SafeFsUnlink[] = "SafeFsUnlink(path)";
static const char rnd_acth_SafeFsUnlink[] = "Unlink a file from the file system. Return value is the same as unlink(2)'s";
static fgw_error_t rnd_act_SafeFsUnlink(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsUnlink, path = argv[1].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_unlink(RND_ACT_DESIGN, path);
	return 0;
}

static const char rnd_acts_SafeFsMkdir[] = "SafeFsMkdir(path, mode)";
static const char rnd_acth_SafeFsMkdir[] = "Mkdir a file from the file system. If mode is a string, it is converted from octal. Return value is the same as mkdir(2)'s";
static fgw_error_t rnd_act_SafeFsMkdir(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;
	int mode = 0;

	if (argc != 3) {
		RND_ACT_FAIL(SafeFsMkdir);
		return FGW_ERR_ARG_CONV;
	}

	RND_ACT_CONVARG(1, FGW_STR, SafeFsMkdir, path = argv[1].val.str);
	if ((argv[2].type & FGW_STR) == FGW_STR) {
		char *end;
		mode = strtol(argv[2].val.str, &end, 8);

		if (*end != '\0') {
			RND_ACT_FAIL(SafeFsMkdir);
			rnd_message(RND_MSG_ERROR, "(Invalid octagonal number in string for the mode argument)\n");
			return FGW_ERR_ARG_CONV;
		}
	}
	else {
		RND_ACT_CONVARG(2, FGW_LONG, SafeFsMkdir, mode = argv[2].val.nat_long);
	}

	res->type = FGW_INT;
	res->val.nat_int = rnd_mkdir(RND_ACT_DESIGN, path, mode);
	return 0;
}

static const char rnd_acts_SafeFsRename[] = "SafeFsRename(old_path, new_path)";
static const char rnd_acth_SafeFsRename[] = "Rename an object on the file system. Return value is the same as rename(2)'s";
static fgw_error_t rnd_act_SafeFsRename(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *old_path, *new_path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsRename, old_path = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, SafeFsRename, new_path = argv[2].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_rename(RND_ACT_DESIGN, old_path, new_path);
	return 0;
}


static const char rnd_acts_SafeFsFileSize[] = "SafeFsFileSize(path)";
static const char rnd_acth_SafeFsFileSize[] = "Return the size of a file in bytes, or -1 on error.";
static fgw_error_t rnd_act_SafeFsFileSize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsFileSize, path = argv[1].val.str);

	res->type = FGW_LONG;
	res->val.nat_long = rnd_file_mtime(RND_ACT_DESIGN, path);
	return 0;
}

static const char rnd_acts_SafeFsFileMtime[] = "SafeFsFileMtime(path)";
static const char rnd_acth_SafeFsFileMtime[] = "Return the last modification time of a file, from Epoch, or -1 on error.";
static fgw_error_t rnd_act_SafeFsFileMtime(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsFileMtime, path = argv[1].val.str);

	res->type = FGW_DOUBLE;
	res->val.nat_double = rnd_file_mtime(RND_ACT_DESIGN, path);
	return 0;
}


static const char rnd_acts_SafeFsIsDir[] = "SafeFsIsDir(path)";
static const char rnd_acth_SafeFsIsDir[] = "Return 1 if path exists and is a directory, else return 0.";
static fgw_error_t rnd_act_SafeFsIsDir(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsIsDir, path = argv[1].val.str);

	res->type = FGW_INT;
	res->val.nat_int = rnd_is_dir(RND_ACT_DESIGN, path);
	return 0;
}

static const char rnd_acts_SafeFsPathSep[] = "SafeFsPathSep(path)";
static const char rnd_acth_SafeFsPathSep[] = "Return the system dependet path separator character (normally slash).";
static fgw_error_t rnd_act_SafeFsPathSep(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	res->type = FGW_STR;
	res->val.cstr = RND_DIR_SEPARATOR_S;
	return 0;
}

static const char rnd_acts_SafeFsRealPath[] = "SafeFsRealPath(path)";
static const char rnd_acth_SafeFsRealPath[] = "Returns the realpath(3) of path, or NULL on error.";
static fgw_error_t rnd_act_SafeFsRealPath(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsRealPath, path = argv[1].val.str);

	res->type = FGW_STR | FGW_DYN;
	res->val.str = rnd_lrealpath(path);
	return 0;
}

static rnd_action_t rnd_safe_fs_action_list[] = {
	{"SafeFsSystem", rnd_act_SafeFsSystem, rnd_acth_SafeFsSystem, rnd_acts_SafeFsSystem},
	{"SafeFsRemove", rnd_act_SafeFsRemove, rnd_acth_SafeFsRemove, rnd_acts_SafeFsRemove},
	{"SafeFsUnlink", rnd_act_SafeFsUnlink, rnd_acth_SafeFsUnlink, rnd_acts_SafeFsUnlink},
	{"SafeFsMkdir", rnd_act_SafeFsMkdir, rnd_acth_SafeFsMkdir, rnd_acts_SafeFsMkdir},
	{"SafeFsRename", rnd_act_SafeFsRename, rnd_acth_SafeFsRename, rnd_acts_SafeFsRename},
	{"SafeFsFileSize", rnd_act_SafeFsFileSize, rnd_acth_SafeFsFileSize, rnd_acts_SafeFsFileSize},
	{"SafeFsFileMtime", rnd_act_SafeFsFileMtime, rnd_acth_SafeFsFileMtime, rnd_acts_SafeFsFileMtime},
	{"SafeFsIsDir", rnd_act_SafeFsIsDir, rnd_acth_SafeFsIsDir, rnd_acts_SafeFsIsDir},
	{"SafeFsPathSep", rnd_act_SafeFsPathSep, rnd_acth_SafeFsPathSep, rnd_acts_SafeFsPathSep},
	{"SafeFsRealPath", rnd_act_SafeFsRealPath, rnd_acth_SafeFsRealPath, rnd_acts_SafeFsRealPath}
};

void rnd_safe_fs_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_safe_fs_action_list, NULL);
}



