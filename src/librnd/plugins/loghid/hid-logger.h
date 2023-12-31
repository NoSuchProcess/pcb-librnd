#ifndef RND_HID_LOGGER_H
#define RND_HID_LOGGER_H

#include <librnd/hid/hid.h>

#include <stdio.h>

/* Set up the delegating loghid that sends all calls to the delegatee but also
   logs the calls. */
void create_log_hid(FILE *log_out, rnd_hid_t *loghid, rnd_hid_t *delegatee);

#endif
