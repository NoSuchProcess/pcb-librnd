/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2004, 2006 Dan McMahill
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

#include <librnd/core/compat_inc.h>
#include <librnd/rnd_config.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>
#include <genvector/gds_char.h>
#include <librnd/core/compat_misc.h>
#include <genht/htss.h>
#include <genht/hash.h>

/* On some old systems random() works better than rand(). Unfortunately
random() is less portable than rand(), which is C89. By default, just
use rand(). Later on: scconfig should detect and enable random() if
we find a system where it really breaks. */
#ifdef HAVE_RANDOM
long rnd_rand(void)
{
	return (long) random();
}
#else
long rnd_rand(void)
{
	return (long) rand();
}
#endif

const char *rnd_get_user_name(void)
{
#ifdef RND_HAVE_GETPWUID
	static struct passwd *pwentry;

	int len;
	char *comma, *gecos, *fab_author;

	/* ID the user. */
	pwentry = getpwuid(getuid());
	gecos = pwentry->pw_gecos;
	comma = strchr(gecos, ',');
	if (comma)
		len = comma - gecos;
	else
		len = strlen(gecos);
	fab_author = (char *) malloc(len + 1);
	if (!fab_author) {
		perror("pcb: out of memory.\n");
		exit(-1);
	}
	memcpy(fab_author, gecos, len);
	fab_author[len] = 0;
	return fab_author;
#else
	return "Unknown";
#endif
}

int rnd_getpid(void)
{
	return getpid();
}

char *rnd_strndup(const char *s, int len)
{
	int a, l = strlen(s);
	char *o;

	a = (len < l) ? len : l;
	o = malloc(a+1);
	memcpy(o, s, a);
	o[a] = '\0';
	return o;
}

char *rnd_strdup(const char *s)
{
	int l = strlen(s);
	char *o;
	o = malloc(l+1);
	memcpy(o, s, l+1);
	return o;
}

#ifdef RND_HAVE_ROUND
#undef round
extern double round(double x);
double rnd_round(double x)
{
	return round(x);
}
#else

/* Implementation idea borrowed from an old gcc (GPL'd) */
double rnd_round(double x)
{
	double t;

/* We should check for inf here, but inf is not in C89; if we'd have isinf(),
   we'd have round() as well and we wouldn't be here at all. */

	if (x >= 0.0) {
		t = ceil(x);
		if (t - x > 0.5)
			t -= 1.0;
		return t;
	}

	t = ceil(-x);
	if ((t + x) > 0.5)
		t -= 1.0;
	return -t;
}
#endif

int rnd_strcasecmp(const char *s1, const char *s2)
{
	while(tolower(*s1) == tolower(*s2)) {
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

int rnd_strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n == 0)
		return 0;

	while(tolower(*s1) == tolower(*s2)) {
		n--;
		if (n == 0)
			return 0;
		if (*s1 == '\0')
			return 0;
		s1++;
		s2++;
	}
	return tolower(*s1) - tolower(*s2);
}

#ifdef RND_HAVE_SETENV
	extern int setenv();
#endif

int rnd_setenv(const char *name, const char *val, int overwrite)
{
#ifdef RND_HAVE_SETENV
	return setenv(name, val, overwrite);
#else
#	ifdef RND_HAVE_PUTENV
	static htss_t cache; /* not freed on exit - acceptable leak because it is all the env */
	static int cache_inited = 0;
	htss_entry_t *e;
	int res;
	gds_t tmp;

	if (!cache_inited) {
		htss_init(&cache, strhash, strkeyeq);
		cache_inited = 1;
	}

	gds_init(&tmp);
	gds_append_str(&tmp, name);
	gds_append(&tmp, '=');
	gds_append_str(&tmp, val);
	res = putenv(tmp.array);

	/* free previous value */
	e = htss_popentry(&cache, name);
	if (e != NULL) {
		free(e->key);
		free(e->value);
	}

	/* remember new value so it can be freed later */
	htss_set(&cache, rnd_strdup(name), tmp.array);

	return res;
#	else
	return -1;
#	endif
#endif
}

size_t rnd_print_utc(char *out, size_t out_len, time_t when)
{
	static const char *fmt = "%Y-%m-%d %H:%M:%S UTC";
	if (when <= 0)
		when = time(NULL);

	return strftime(out, out_len, fmt, gmtime(&when));
}

void rnd_ms_sleep(long ms)
{
#ifdef RND_HAVE_USLEEP
	usleep(ms*1000);
#else
#	ifdef RND_HAVE_WSLEEP
		Sleep(ms);
#	else
#		ifdef RND_HAVE_SELECT
			fd_set s;
			struct timeval tv;

			FD_ZERO(&s);
			tv.tv_sec  = 0;
			tv.tv_usec = ms*1000;
			select(0, &s, &s, &s, &tv);
#		else
#			error rnd_ms_sleep(): no milisecond sleep on this host.
#		endif
#	endif
#endif
}

void rnd_ltime(unsigned long  *secs, unsigned long *usecs)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*secs  = tv.tv_sec;
	if (usecs != NULL)
		*usecs = tv.tv_usec;
}

double rnd_dtime(void)
{
 unsigned long  s, u;

	rnd_ltime(&s, &u);
	return (double)u / 1000000.0 + (double)s;
}

int rnd_fileno(FILE *f)
{
	return RND_HOST_FILENO(f);
}
