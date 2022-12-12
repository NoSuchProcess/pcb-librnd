/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019,2020 Tibor 'Igor2' Palinkas
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

#include <errno.h>
#include <stdarg.h>

#include <librnd/core/actions.h>
#include <librnd/core/error.h>
#include <librnd/core/event.h>
#include <librnd/hid/hid_dad.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/rnd_conf.h>
#include <genvector/gds_char.h>

rnd_hid_t *rnd_gui = NULL;

void rnd_trace(const char *Format, ...)
{
#ifndef NDEBUG
	va_list args;
	va_start(args, Format);
	rnd_vfprintf(stderr, Format, args);
	va_end(args);
#endif
}

unsigned long rnd_log_next_ID = 0;
rnd_logline_t *rnd_log_first, *rnd_log_last;


/* allocate a new log line; for efficiency pretend it is a string during
   the allocation */
#define RND_MSG_PREPARE_BUF(buf) \
	do { \
		gds_enlarge(buf, sizeof(rnd_logline_t)); \
		tmp.used = offsetof(rnd_logline_t, str); \
	} while(0)

RND_INLINE void rnd_message_(rnd_message_level_t level, gds_t *buf)
{
	rnd_message_level_t min_level = RND_MSG_INFO;
	rnd_logline_t *line;

	if ((rnd_gui == NULL) || (rnd_conf.rc.dup_log_to_stderr)) {
		if (rnd_conf.rc.quiet)
			min_level = RND_MSG_ERROR;

		if ((level >= min_level) || (rnd_conf.rc.verbose))
			rnd_fprintf(stderr, "%s", buf->array + offsetof(rnd_logline_t, str));
	}

	/* add the header and link in */
	line = (rnd_logline_t *)buf->array;
	line->stamp = time(NULL);
	line->ID = rnd_log_next_ID++;
	line->level = level;
	line->seen = 0;
	line->next = NULL;
	line->prev = rnd_log_last;
	if (rnd_log_first == NULL)
		rnd_log_first = line;
	if (rnd_log_last != NULL)
		rnd_log_last->next = line;
	rnd_log_last = line;
	line->len = buf->used - offsetof(rnd_logline_t, str);

	rnd_event(NULL, RND_EVENT_LOG_APPEND, "p", line);
}

RND_INLINE void rnd_vmessage_(rnd_message_level_t level, const char *Format, va_list args)
{
	gds_t tmp = {0};
	RND_MSG_PREPARE_BUF(&tmp);

	rnd_safe_append_vprintf(&tmp, 0, Format, args);
	rnd_message_(level, &tmp);
}

void rnd_message(rnd_message_level_t level, const char *Format, ...)
{
	va_list args;

	va_start(args, Format);
	rnd_vmessage_(level, Format, args);
	va_end(args);
}

void rnd_vmessage(rnd_message_level_t level, const char *Format, va_list args)
{
	rnd_vmessage_(level, Format, args);
}

void rnd_msg_error(const char *Format, ...)
{
	va_list args;

	va_start(args, Format);
	rnd_vmessage_(RND_MSG_ERROR, Format, args);
	va_end(args);
}

void rnd_msg_warning(const char *Format, ...)
{
	va_list args;

	va_start(args, Format);
	rnd_vmessage_(RND_MSG_WARNING, Format, args);
	va_end(args);
}


rnd_logline_t *rnd_log_find_min_(rnd_logline_t *from, unsigned long ID)
{
	rnd_logline_t *n;
	if (ID == -1)
		return from;
	for(n = from; n != NULL; n = n->next)
		if (n->ID >= ID)
			return n;
	return NULL;
}

rnd_logline_t *rnd_log_find_min(unsigned long ID)
{
	return rnd_log_find_min_(rnd_log_first, ID);
}

rnd_logline_t *rnd_log_find_first_unseen(void)
{
	rnd_logline_t *n;
	for(n = rnd_log_last; n != NULL; n = n->prev)
		if (n->seen)
			return n->next;
	return rnd_log_first;
}

void rnd_log_del_range(unsigned long from, unsigned long to)
{
	rnd_logline_t *start = rnd_log_find_min(from), *end = NULL;
	rnd_logline_t *next, *n, *start_prev, *end_next;

	if (start == NULL)
		return; /* start is beyond the end of the list - do not delete anything */

	if (to != -1)
		end = rnd_log_find_min_(start, to);
	if (end == NULL)
		end = rnd_log_last;

	/* remember boundary elems near the range */
	start_prev = start->prev;
	end_next = end->next;

	/* free all elems of the range */
	n = start;
	for(;;) {
		next = n->next;
		free(n);
		if (n == end)
			break;
		n = next;
	}

	/* unlink the whole range at once */
	if (start_prev != NULL)
		start_prev->next = end_next;
	else
		rnd_log_first = end_next;

	if (end_next != NULL)
		end_next->prev = start_prev;
	else
		rnd_log_last = start_prev;
}



int rnd_log_export(rnd_design_t *hidlib, const char *fn, int fmt_lihata)
{
	FILE *f;
	rnd_logline_t *n;

	f = rnd_fopen(hidlib, fn, "w");
	if (f == NULL)
		return -1;

	if (fmt_lihata) {
		fprintf(f, "ha:%s-log-v1 {\n", rnd_app.package);
		fprintf(f, " li:entries {\n");
	}

	for(n = rnd_log_first; n != NULL; n = n->next) {
		if (fmt_lihata) {
			fprintf(f, "  ha:%lu {", n->ID);
			fprintf(f, "stamp=%ld; level=%d; seen=%d; ", (long int)n->stamp, n->level, n->seen);
			fprintf(f, "str={%s}}\n", n->str);
		}
		else
			fprintf(f, "%s", n->str);
	}

	if (fmt_lihata)
		fprintf(f, " }\n}\n");

	fclose(f);
	return 0;
}

void rnd_log_uninit(void)
{
	rnd_log_del_range(-1, -1);
}


static const char rnd_acts_Log[] =
	"Log(clear, [fromID, [toID])\n"
	"Log(export, [filename, [text|lihata])\n";
static const char rnd_acth_Log[] = "Manages the central, in-memory log.";
static fgw_error_t rnd_act_Log(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int ret;
	const char *op = "";

	RND_ACT_MAY_CONVARG(1, FGW_STR, Log, op = argv[1].val.str);

	if (rnd_strcasecmp(op, "Clear") == 0) {
		unsigned long from = -1, to = -1;
		RND_ACT_MAY_CONVARG(2, FGW_ULONG, Log, from = fgw_keyword(&argv[2]));
		RND_ACT_MAY_CONVARG(3, FGW_ULONG, Log, to = fgw_keyword(&argv[3]));
		rnd_log_del_range(from, to);
		rnd_event(NULL, RND_EVENT_LOG_CLEAR, "pp", &from, &to);
		ret = 0;
	}
	else if (rnd_strcasecmp(op, "Export") == 0) {
		const char *fn = NULL, *fmt = NULL;

		RND_ACT_MAY_CONVARG(2, FGW_STR, Log, fn = argv[2].val.str);
		RND_ACT_MAY_CONVARG(3, FGW_STR, Log, fmt = argv[3].val.str);

		if (fn != NULL) {
			int fmt_lihata = (fmt != NULL) && (strcmp(fmt, "lihata") == 0);
			ret = rnd_log_export(NULL, fn, fmt_lihata);
			if (ret != 0)
				rnd_message(RND_MSG_ERROR, "Failed to export log to '%s'\n", fn);
		}
		else {
			/* call through to the dialog box version */
			return rnd_actionv_bin(RND_ACT_DESIGN, "LogGui", res, argc, argv);
		}
	}
	else {
		RND_ACT_FAIL(Log);
		ret = -1;
	}

	RND_ACT_IRES(ret);
	return 0;
}

static const char rnd_acts_Message[] = "message([ERROR|WARNING|INFO|DEBUG,] message)";
static const char rnd_acth_Message[] = "Writes a message to the log window.";
/* DOC: message.html */
static fgw_error_t rnd_act_Message(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int i, how = RND_MSG_INFO;

	if (argc < 2)
		RND_ACT_FAIL(Message);

	i = 1;
	if (argc > 2) {
		const char *hows;
		RND_ACT_MAY_CONVARG(i, FGW_STR, Message, hows = argv[i].val.str);
		if (strcmp(hows, "ERROR") == 0)        { i++; how = RND_MSG_ERROR; }
		else if (strcmp(hows, "WARNING") == 0) { i++; how = RND_MSG_WARNING; }
		else if (strcmp(hows, "INFO") == 0)    { i++; how = RND_MSG_INFO; }
		else if (strcmp(hows, "DEBUG") == 0)   { i++; how = RND_MSG_DEBUG; }
	}

	RND_ACT_IRES(0);
	for(; i < argc; i++) {
		char *s = NULL;
		RND_ACT_MAY_CONVARG(i, FGW_STR, Message, s = argv[i].val.str);
		if ((s != NULL) && (*s != '\0'))
			rnd_message(how, s);
		rnd_message(how, "\n");
	}

	return 0;
}

static rnd_action_t log_action_list[] = {
	{"Log", rnd_act_Log, rnd_acth_Log, rnd_acts_Log},
	{"Message", rnd_act_Message, rnd_acth_Message, rnd_acts_Message}
};

void rnd_hidlib_error_init2(void)
{
	RND_REGISTER_ACTIONS(log_action_list, NULL);
}
