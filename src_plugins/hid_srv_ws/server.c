/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Websocket server HID: listen for connections and fork session servers */

/* Needed by libuv to compile in C99 */
#define _POSIX_C_SOURCE 200112

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <libwebsockets.h>

#include "plugins.h"
#include "error.h"

int pplg_check_ver_hid_srv_ws(int ver_needed) { return 0; }

extern int pplg_init_hid_remote(void);
extern void pcb_set_hid_name(const char *new_hid_name);

/* Number of sockets to handle - need one listening socket and one accepted
   because each client is forked; leave some room for retries and whatnot */
#define WS_MAX_FDS 8

typedef struct {
	struct lws_context *context;
	struct lws_pollfd pollfds[WS_MAX_FDS];
	int fd_lookup[WS_MAX_FDS];
	int count_pollfds;
	int listen_fd;
	pid_t pid;
} hid_srv_ws_t;

static hid_srv_ws_t hid_srv_ws_ctx;

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	int n;
	struct lws_pollargs *pa = (struct lws_pollargs *)in;
/*	hid_srv_ws_t *ctx = lws_wsi_user(wsi); why does this return NULL???? */
	hid_srv_ws_t *ctx = &hid_srv_ws_ctx;

	switch (reason) {
		case LWS_CALLBACK_ADD_POLL_FD:
			if (ctx->count_pollfds >= WS_MAX_FDS) {
				lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
				return 1;
			}

			/* assume the first fd added via this callback is the listener */
			if (ctx->listen_fd < 0)
				ctx->listen_fd = pa->fd;

			ctx->fd_lookup[pa->fd] = ctx->count_pollfds;
			ctx->pollfds[ctx->count_pollfds].fd = pa->fd;
			ctx->pollfds[ctx->count_pollfds].events = pa->events;
			ctx->pollfds[ctx->count_pollfds].revents = 0;
			ctx->count_pollfds++;
			break;

		case LWS_CALLBACK_DEL_POLL_FD:
			/* the server process has 1 listening socket; the client process has 1
			   client sokcet. Any socket close means the main task is over. */
			pcb_message(PCB_MSG_INFO, "websocket [%d]: connection closed, exiting\n", ctx->pid);
			exit(0);

		case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
			ctx->pollfds[ctx->fd_lookup[pa->fd]].events = pa->events;
			break;
			
		default:
			break;
	}

	return 0;
}


static int callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
	hid_srv_ws_t *ctx = &hid_srv_ws_ctx;

	switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: /* just log message that someone is connecting */
			pcb_message(PCB_MSG_INFO, "websocket [%d]: high level connection established\n", ctx->pid);
			break;
			
		case LWS_CALLBACK_RECEIVE: {
			/* the funny part
			   create a buffer to hold our response
			   it has to have some pre and post padding. You don't need to care
			   what comes there, lwss will do everything for you. For more info see
			   http://git.warmcat.com/cgi-bin/cgit/lwss/tree/lib/lwss.h#n597 */
			unsigned char *buf = (unsigned char*) malloc(LWS_SEND_BUFFER_PRE_PADDING + len + LWS_SEND_BUFFER_POST_PADDING);
			int i;

			/* pointer to `void *in` holds the incomming request
			   we're just going to put it in reverse order and put it in `buf` with
			   correct offset. `len` holds length of the request. */
			for (i=0; i < len; i++) {
				buf[LWS_SEND_BUFFER_PRE_PADDING + (len - 1) - i ] = ((char *) in)[i];
			}

			/* log what we recieved and what we're going to send as a response.
			   that disco syntax `%.*s` is used to print just a part of our buffer
			   http://stackoverflow.com/questions/5189071/print-part-of-char-array */
			pcb_message(PCB_MSG_INFO, "websocket [%d]: received data: %s, replying: %.*s\n", ctx->pid, (char *)in, (int)len, buf + LWS_SEND_BUFFER_PRE_PADDING);

			/* send response
			   just notice that we have to tell where exactly our response starts. That's
			   why there's `buf[LWS_SEND_BUFFER_PRE_PADDING]` and how long it is.
			   we know that our response has the same length as request because
			   it's the same message in reverse order. */
			lws_write(wsi, &buf[LWS_SEND_BUFFER_PRE_PADDING], len, LWS_WRITE_TEXT);

			free(buf);
			break;
			}
			
		case LWS_CALLBACK_CLOSED:
			pcb_message(PCB_MSG_INFO, "websocket [%d]: connection closed\n", ctx->pid);
			break;
			
		default:
			break;
	}

	return 0;
}


static struct lws_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",   /* name */
		callback_http, /* callback */
		0,             /* per_session_data_size */
		0              /* max frame size / rx buffer */
	},
	{
		"dumb-increment-protocol", /* protocol name - very important! */
		callback_dumb_increment,   /* callback */
		0,                         /* we don't use any per session data */
		0                          /* max frame size / rx buffer */
	},
	{ NULL, NULL, 0, 0 }   /* End of list */
};


static int src_ws_mainloop(hid_srv_ws_t *ctx)
{
	unsigned int ms, ms_1sec = 0;

	for(;;) {
		struct timeval tv;
		int n;

		gettimeofday(&tv, NULL);
		ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

		n = poll(ctx->pollfds, ctx->count_pollfds, 50);

		if (n < 0)
			continue;

		if (n != 0) {
			for(n = 0; n < ctx->count_pollfds; n++) {
				if (ctx->pollfds[n].revents) {
					if (ctx->pollfds[n].fd == ctx->listen_fd) {
						/* listening socket: fork for each client */
						pid_t p;
						
						p = fork();
						if (p == 0) {
							/* child: client */
							if (lws_service_fd(ctx->context, &ctx->pollfds[n]) < 0)
								return -1;
							close(ctx->pollfds[n].fd);
							ctx->pollfds[n].fd = -ctx->pollfds[n].fd;
							ctx->pid = getpid();
							pcb_message(PCB_MSG_INFO, "websocket [%d]: new client conn accepted\n", ctx->pid);
						}
						else {
							/* parent: announce the fork but don't do anything else */
							pcb_message(PCB_MSG_INFO, "websocket [%d]: new client conn forked\n", ctx->pid);
						}
#warning TODO: rather communicate back from the client
						sleep(1);
					}
					else {
						/* returns immediately if the fd does not match anything under libwebsocketscontrol */
						if (lws_service_fd(ctx->context, &ctx->pollfds[n]) < 0)
							return -1;
					}
				}
			}
			
			/* if needed, force-service wsis that may not have read all input */
			/* NOTE: lws_service_adjust_timeout() is not available in v2.0 (version of package in debian) */
			/*
			while (!lws_service_adjust_timeout(context, 1, 0)) {
				lwsl_notice("extpoll doing forced service!\n");
				lws_service_tsi(context, -1, 0);
			}
			*/
		} else {
			/* no revents, but before polling again, make lws check for any timeouts */
			if (ms - ms_1sec > 1000) {
				lws_service_fd(ctx->context, NULL);
				ms_1sec = ms;
			}
		}
	}
	return 0;
}


static int srv_ws_listen(hid_srv_ws_t *ctx)
{

#warning TODO: get these from the conf
	int port = 9000;
	struct lws_context_creation_info context_info = {
		.port = port, .iface = NULL, .protocols = protocols, .extensions = NULL,
#ifdef WANT_SSL
		.ssl_cert_filepath = "cert.pem", .ssl_private_key_filepath = "key.pem", .ssl_ca_filepath = NULL,
		.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT,
		.gid = -1, .uid = -1, .ka_time = 0, .ka_probes = 0, .ka_interval = 0,
#else
		.ssl_cert_filepath = NULL, .ssl_private_key_filepath = NULL, .ssl_ca_filepath = NULL,
		.gid = -1, .uid = -1, .options = 0, NULL, .ka_time = 0, .ka_probes = 0, .ka_interval = 0,
#endif
		.user = ctx
	};

	ctx->listen_fd = -1;
	ctx->pid = getpid();

	/* create lws context representing this server */
	ctx->context = lws_create_context(&context_info);
	if (ctx->context == NULL) {
		pcb_message(PCB_MSG_ERROR, "lws_create_context() failed\n");
		return -1;
	}

	pcb_message(PCB_MSG_INFO, "websockets [%d]: listening for clients\n", ctx->pid);

	return 0;
}


void pplg_uninit_hid_srv_ws(void)
{
}

int pplg_init_hid_srv_ws(void)
{
	printf("WS: waiting for connections\n");
	pcb_set_hid_name("remote");
	if (srv_ws_listen(&hid_srv_ws_ctx) != 0)
		return -1;

	return src_ws_mainloop(&hid_srv_ws_ctx);
}
