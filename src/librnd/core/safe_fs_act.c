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
	res->val.nat_long = rnd_file_size(RND_ACT_DESIGN, path);
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

static const char rnd_acts_SafeFsReadFile[] = "SafeFsReadFile(path, [maxlen])";
static const char rnd_acth_SafeFsReadFile[] = "Reads a text file into one long string, returned, but at most maxlen bytes. If maxlen is not specified, 64k is used. Returns nil on error or empty file.";
static fgw_error_t rnd_act_SafeFsReadFile(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path;
	long maxlen = 65536, len, rlen;
	FILE *f;
	char *buff = NULL;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsReadFile, path = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_LONG, SafeFsReadFile, maxlen = argv[2].val.nat_long);

	len = rnd_file_size(RND_ACT_DESIGN, path);
	if (maxlen < len)
		len = maxlen;

	if (len > 0) {
		f = rnd_fopen(RND_ACT_DESIGN, path, "r");
		if (f != NULL) {
			buff = malloc(len+1);
			rlen = fread(buff, 1, len, f);
			if (rlen <= 0) {
				free(buff);
				buff = NULL;
			}
			else
				buff[rlen] = '\0';
			
			fclose(f);
		}
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = buff;
	return 0;
}


static const char *PTR_DOMAIN_FILE = "librnd/core/safe_fs_act.c:FILE*";

static const char rnd_acts_SafeFsFopen[] = "SafeFsFopen(path, [mode])";
static const char rnd_acth_SafeFsFopen[] = "Opens a file using fopen, returns FILE *. If mode is not specified, r is assumed. Returns nil on error.";
static fgw_error_t rnd_act_SafeFsFopen(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *path, *mode = "r";
	FILE *f;

	RND_ACT_CONVARG(1, FGW_STR, SafeFsFopen, path = argv[1].val.str);
	RND_ACT_CONVARG(2, FGW_STR, SafeFsFopen, mode = argv[2].val.str);

	f = rnd_fopen(RND_ACT_DESIGN, path, mode);
	if (f == NULL) {
		res->type = FGW_STR;
		res->val.str = NULL;
		return 0;
	}

	fgw_ptr_reg(&rnd_fgw, res, PTR_DOMAIN_FILE, FGW_PTR | FGW_STRUCT, f);
	return 0;
}

static const char rnd_acts_SafeFsFclose[] = "SafeFsFclose(f)";
static const char rnd_acth_SafeFsFclose[] = "Closes a file previously open using SafeFsFopen()";
static fgw_error_t rnd_act_SafeFsFclose(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFclose, ;);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	fclose(argv[1].val.ptr_void);
	fgw_ptr_unreg(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE);
	return 0;
}

static const char rnd_acts_SafeFsFgets[] = "SafeFsFgets(f, [maxlen])";
static const char rnd_acth_SafeFsFgets[] = "Reads and returns a line from f (open with SafeFsFopen()). Stops reading after maxlen (subsequent call will continue reading the same line). Returns nil on error or eof or empty line. Maxlen is 64k by default. Note: string heap allocation is made for maxlen.";
static fgw_error_t rnd_act_SafeFsFgets(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long len = 65536;
	char *buff;

	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFgets, ;);
	RND_ACT_MAY_CONVARG(2, FGW_LONG, SafeFsFgets, len = argv[2].val.nat_long);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	buff = malloc(len+1);
	if (buff != NULL) {
		char *r = fgets(buff, len, argv[1].val.ptr_void);
		if (r == NULL) {
			res->type = FGW_STR;
			res->val.str = NULL;
			return 0;
		}
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = buff;
	return 0;
}

static const char rnd_acts_SafeFsFputs[] = "SafeFsFputs(f, str)";
static const char rnd_acth_SafeFsFputs[] = "Writes str to file f (previously opne with SafeFsFopen))";
static fgw_error_t rnd_act_SafeFsFputs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *str;

	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFputs, ;);
	RND_ACT_CONVARG(2, FGW_STR, SafeFsFputs, str = argv[2].val.str);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	fputs(str, argv[1].val.ptr_void);

	return 0;
}


static const char rnd_acts_SafeFsFread[] = "SafeFsFread(f, len)";
static const char rnd_acth_SafeFsFread[] = "Reads and returns at most len bytes from a file (open with SafeFsFopen()). Returns nil on error or eof or empty line.";
static fgw_error_t rnd_act_SafeFsFread(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	long len;
	char *buff;

	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFread, ;);
	RND_ACT_CONVARG(2, FGW_LONG, SafeFsFread, len = argv[2].val.nat_long);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	buff = malloc(len+1);
	if (buff != NULL) {
		long rlen = fread(buff, 1, len, argv[1].val.ptr_void);
		if (rlen < 0) {
			res->type = FGW_STR;
			res->val.str = NULL;
			return 0;
		}
		buff[rlen] = '\0';
	}

	res->type = FGW_STR | FGW_DYN;
	res->val.str = buff;
	return 0;
}

static const char rnd_acts_SafeFsFeof[] = "SafeFsFeof(f)";
static const char rnd_acth_SafeFsFeof[] = "Returns 1 if file has reached EOF, 0 otherwise";
static fgw_error_t rnd_act_SafeFsFeof(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFeof, ;);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	res->type = FGW_INT;
	res->val.nat_int = feof(argv[1].val.ptr_void);

	return 0;
}

static const char rnd_acts_SafeFsFseek[] = "SafeFsFseek(f, offs, [whence])";
static const char rnd_acth_SafeFsFseek[] = "Same as fseek(3); whence is a string, one of set, cur or end not specified (set is used when not specified)";
static fgw_error_t rnd_act_SafeFsFseek(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	char *whences = "set";
	long offs;
	int whence;

	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFseek, ;);
	RND_ACT_CONVARG(2, FGW_LONG, SafeFsFseek, offs = argv[2].val.nat_long);
	RND_ACT_MAY_CONVARG(3, FGW_STR, SafeFsFseek, whences = argv[3].val.str);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	switch(whences[0]) {
		case 's': case 'S': whence = SEEK_SET; break;
		case 'c': case 'C': whence = SEEK_CUR; break;
		case 'e': case 'E': whence = SEEK_END; break;
		default:
			RND_ACT_FAIL(SafeFsMkdir);
			return FGW_ERR_ARG_CONV;
	}

	res->type = FGW_INT;
	res->val.nat_int = fseek(argv[1].val.ptr_void, offs, whence);
	return 0;
}

static const char rnd_acts_SafeFsFtell[] = "SafeFsFtell(f)";
static const char rnd_acth_SafeFsFtell[] = "Same as ftell(3).";
static fgw_error_t rnd_act_SafeFsFtell(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsFtell, ;);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	res->type = FGW_LONG;
	res->val.nat_int = ftell(argv[1].val.ptr_void);

	return 0;
}

static const char rnd_acts_SafeFsRewind[] = "SafeFsRewind(f)";
static const char rnd_acth_SafeFsRewind[] = "Same as rewind(3)";
static fgw_error_t rnd_act_SafeFsRewind(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	RND_ACT_CONVARG(1, FGW_PTR | FGW_STRUCT, SafeFsRewind, ;);

	if ((((argv[1].type & FGW_PTR) != FGW_PTR)) || (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], PTR_DOMAIN_FILE)))
		return FGW_ERR_PTR_DOMAIN;

	rewind(argv[1].val.ptr_void);
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
	{"SafeFsRealPath", rnd_act_SafeFsRealPath, rnd_acth_SafeFsRealPath, rnd_acts_SafeFsRealPath},
	{"SafeFsReadFile", rnd_act_SafeFsReadFile, rnd_acth_SafeFsReadFile, rnd_acts_SafeFsReadFile},
	{"SafeFsFopen", rnd_act_SafeFsFopen, rnd_acth_SafeFsFopen, rnd_acts_SafeFsFopen},
	{"SafeFsFclose", rnd_act_SafeFsFclose, rnd_acth_SafeFsFclose, rnd_acts_SafeFsFclose},
	{"SafeFsFgets", rnd_act_SafeFsFgets, rnd_acth_SafeFsFgets, rnd_acts_SafeFsFgets},
	{"SafeFsFputs", rnd_act_SafeFsFputs, rnd_acth_SafeFsFputs, rnd_acts_SafeFsFputs},
	{"SafeFsFread", rnd_act_SafeFsFread, rnd_acth_SafeFsFread, rnd_acts_SafeFsFread},
	{"SafeFsFeof", rnd_act_SafeFsFeof, rnd_acth_SafeFsFeof, rnd_acts_SafeFsFeof},
	{"SafeFsFseek", rnd_act_SafeFsFseek, rnd_acth_SafeFsFseek, rnd_acts_SafeFsFseek},
	{"SafeFsFtell", rnd_act_SafeFsFtell, rnd_acth_SafeFsFtell, rnd_acts_SafeFsFtell},
	{"SafeFsRewind", rnd_act_SafeFsRewind, rnd_acth_SafeFsRewind, rnd_acts_SafeFsRewind}
};

void rnd_safe_fs_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_safe_fs_action_list, NULL);
}



