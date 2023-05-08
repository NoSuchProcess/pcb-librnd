/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016,2020,2021,2023 Tibor 'Igor2' Palinkas
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
#include <librnd/core/rnd_conf.h>
#include <librnd/core/funchash_core.h>
#include <librnd/core/error.h>
#include <librnd/core/hidlib.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>

static const char rnd_acts_Conf[] =
	"conf(set, path, value, [role], [policy]) - change a config setting to an absolute value\n"
	"conf(delta, path, value, [role], [policy]) - change a config setting by a delta value (numerics-only)\n"
	"conf(toggle, path, [role]) - invert boolean value of a flag; if no role given, overwrite the highest prio config\n"
	"conf(reset, role) - reset the in-memory lihata of a role\n"
	"conf(iseq, path, value) - returns whether the value of a conf item matches value (for menu checked's)\n"
	;
static const char rnd_acth_Conf[] = "Perform various operations on the configuration tree.";

extern lht_doc_t *conf_root[];
RND_INLINE int conf_iseq_pf(void *ctx, const char *fmt, ...)
{
	int res;
	va_list ap;
	va_start(ap, fmt);
	res = rnd_safe_append_vprintf((gds_t *)ctx, 0, fmt, ap);
	va_end(ap);
	return res;
}

static fgw_error_t rnd_act_Conf(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	const char *a1, *a2, *a3, *a4;

	RND_ACT_CONVARG(1, FGW_KEYWORD, Conf, op = fgw_keyword(&argv[1]));
	RND_ACT_MAY_CONVARG(2, FGW_STR, Conf, a1 = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, Conf, a2 = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, Conf, a3 = argv[4].val.str);
	RND_ACT_MAY_CONVARG(5, FGW_STR, Conf, a4 = argv[5].val.str);

	if ((op == F_Set) || (op == F_Delta)) {
		const char *path, *val;
		char valbuff[128];
		rnd_conf_policy_t pol = RND_POL_OVERWRITE;
		rnd_conf_role_t role = RND_CFR_invalid;
		int rs;

		if (argc < 4) {
			rnd_message(RND_MSG_ERROR, "conf(set) needs at least two arguments");
			return FGW_ERR_ARGC;
		}
		if (argc > 4) {
			role = rnd_conf_role_parse(a3);
			if (role == RND_CFR_invalid) {
				rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", a3);
				return FGW_ERR_ARG_CONV;
			}
		}
		if (argc > 5) {
			pol = rnd_conf_policy_parse(a4);
			if (pol == RND_POL_invalid) {
				rnd_message(RND_MSG_ERROR, "Invalid policy: '%s'", a4);
				return FGW_ERR_ARG_CONV;
			}
		}
		path = a1;
		val = a2;

		if (op == F_Delta) {
			double d;
			char *end;
			rnd_conf_native_t *n = rnd_conf_get_field(a1);

			if (n == 0) {
				rnd_message(RND_MSG_ERROR, "Can't delta-set '%s': no such path\n", argv[1]);
				return FGW_ERR_ARG_CONV;
			}

			switch(n->type) {
				case RND_CFN_REAL:
					d = strtod(val, &end);
					if (*end != '\0') {
						rnd_message(RND_MSG_ERROR, "Can't delta-set '%s': invalid delta value\n", a1);
						return FGW_ERR_ARG_CONV;
					}
					d += *n->val.real;
					sprintf(valbuff, "%f", d);
					val = valbuff;
					break;
				case RND_CFN_COORD:
				case RND_CFN_INTEGER:
				default:
					rnd_message(RND_MSG_ERROR, "Can't delta-set '%s': not a numeric item\n", a1);
					return FGW_ERR_ARG_CONV;
			}
		}

		if (role == RND_CFR_invalid) {
			rnd_conf_native_t *n = rnd_conf_get_field(a1);
			if (n == NULL) {
				rnd_message(RND_MSG_ERROR, "Invalid conf field '%s': no such path\n", a1);
				return FGW_ERR_ARG_CONV;
			}
			rs = rnd_conf_set_native(n, 0, val);
			rnd_conf_update(a1, 0);
		}
		else
			rs = rnd_conf_set(role, path, -1, val, pol);

		if (rs != 0) {
			rnd_message(RND_MSG_ERROR, "conf(set) failed.\n");
			return FGW_ERR_UNKNOWN;
		}
	}

	else if (op == F_Iseq) {
		const char *path, *val;
		int rs;
		gds_t nval;
		rnd_conf_native_t *n;

		if (argc != 4) {
			rnd_message(RND_MSG_ERROR, "conf(iseq) needs two arguments");
			return FGW_ERR_ARGC;
		}
		path = a1;
		val = a2;

		n = rnd_conf_get_field(a1);
		if (n == NULL) {
			if (rnd_conf.rc.verbose)
				rnd_message(RND_MSG_ERROR, "Invalid conf field '%s' in iseq: no such path\n", path);
			return FGW_ERR_ARG_CONV;
		}

		gds_init(&nval);
		rnd_conf_print_native_field(conf_iseq_pf, &nval, 0, &n->val, n->type, NULL, 0);
		rs = !strcmp(nval.array, val);
/*		printf("iseq: %s %s==%s %d\n", path, nval.array, val, rs);*/
		gds_uninit(&nval);

		RND_ACT_IRES(rs);
		return 0;
	}

	else if (op == F_Toggle) {
		rnd_conf_native_t *n = rnd_conf_get_field(a1);
		const char *new_value;
		rnd_conf_role_t role = RND_CFR_invalid;
		int res;

		if (n == NULL) {
			rnd_message(RND_MSG_ERROR, "Invalid conf field '%s': no such path\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		if (n->type != RND_CFN_BOOLEAN) {
			rnd_message(RND_MSG_ERROR, "Can not toggle '%s': not a boolean\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		if (n->used != 1) {
			rnd_message(RND_MSG_ERROR, "Can not toggle '%s': array size should be 1, not %d\n", a1, n->used);
			return FGW_ERR_UNKNOWN;
		}
		if (argc > 3) {
			role = rnd_conf_role_parse(a2);
			if (role == RND_CFR_invalid) {
				rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", a2);
				return FGW_ERR_ARG_CONV;
			}
		}
		if (n->val.boolean[0])
			new_value = "false";
		else
			new_value = "true";
		if (role == RND_CFR_invalid)
			res = rnd_conf_set_native(n, 0, new_value);
		else
			res = rnd_conf_set(role, a1, -1, new_value, RND_POL_OVERWRITE);

		if (res != 0) {
			rnd_message(RND_MSG_ERROR, "Can not toggle '%s': failed to set new value\n", a1);
			return FGW_ERR_UNKNOWN;
		}
		rnd_conf_update(a1, -1);
	}

	else if (op == F_Reset) {
		rnd_conf_role_t role;
		role = rnd_conf_role_parse(a1);
		if (role == RND_CFR_invalid) {
			rnd_message(RND_MSG_ERROR, "Invalid role: '%s'", a1);
			return FGW_ERR_ARG_CONV;
		}
		rnd_conf_reset(role, "<action>");
		rnd_conf_update(a1, -1);
	}

	else {
		rnd_message(RND_MSG_ERROR, "Invalid conf command\n");
		return FGW_ERR_ARG_CONV;
	}

	RND_ACT_IRES(0);
	return 0;
}

static const char rnd_acts_ConfGet[] =
	"ConfGet(path, [value]) - return the value of a conf node; units, colors and lists are returned as string.\n"
	"ConfGet(path, desc) - return the human readable description\n"
	"ConfGet(path, ArraySize) - number of elements\n"
	"ConfGet(path, ReadOnly) - returns 1 if node is read-only, 0 otherwise\n"
	"ConfGet(path, ConfRev) - conf (merge) rev number the value last changed\n"
	;
static const char rnd_acth_ConfGet[] = "Return conf node value or property from the merged in-memory/native storage. Returns nil if node is unset (for value query) or not found. Intended for scripting.";
static fgw_error_t rnd_act_ConfGet(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op = F_Value;
	long idx = 0;
	const char *path;
	rnd_conf_native_t *nat;

	RND_ACT_CONVARG(1, FGW_STR, ConfGet, path = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_KEYWORD, ConfGet, op = fgw_keyword(&argv[2]));

	/* prepare to return nil by default */
	res->type = FGW_PTR;
	res->val.ptr_void = NULL;

	nat = rnd_conf_get_field(path);
	if (nat == NULL)
		return 0;

	switch(op) {
		case F_Value:
			switch(nat->type) {
				case RND_CFN_STRING:
					res->type = FGW_STR;
					res->val.cstr = nat->val.string[idx];
					break;
				case RND_CFN_BOOLEAN:
					res->type = FGW_INT;
					res->val.nat_int = nat->val.boolean[idx];
					break;
				case RND_CFN_INTEGER:
					res->type = FGW_LONG;
					res->val.nat_long = nat->val.integer[idx];
					break;
				case RND_CFN_REAL:
					res->type = FGW_DOUBLE;
					res->val.nat_double = nat->val.real[idx];
					break;
				case RND_CFN_COORD:
					res->type = FGW_COORD;
					*(res->val.custom.c) = nat->val.coord[idx];
					break;

				case RND_CFN_UNIT:
				case RND_CFN_COLOR:
				case RND_CFN_LIST:
				case RND_CFN_HLIST:
					{
						gds_t tmp = {0};
						rnd_conf_print_native((rnd_conf_pfn)rnd_append_printf, &tmp, NULL, 0, nat);
						res->type = FGW_STR | FGW_DYN;
						res->val.str = tmp.array;
					}
					break;
				default:
					return 0;
			}
			break;
		case F_Desc:
			res->type = FGW_STR;
			res->val.cstr = nat->description;
			break;
		case F_ArraySize:
			res->type = FGW_LONG;
			res->val.nat_long = nat->array_size;
			break;
		case F_ReadOnly:
			res->type = FGW_INT;
			res->val.nat_long = nat->random_flags.read_only;
			break;
		case F_ConfRev:
			res->type = FGW_LONG;
			res->val.nat_long = nat->rnd_conf_rev;
			break;
	}
	
	return 0;
}


static rnd_action_t rnd_conf_action_list[] = {
	{"conf", rnd_act_Conf, rnd_acth_Conf, rnd_acts_Conf},
	{"confget", rnd_act_ConfGet, rnd_acth_ConfGet, rnd_acts_ConfGet}
};

void rnd_conf_act_init2(void)
{
	RND_REGISTER_ACTIONS(rnd_conf_action_list, NULL);
}



