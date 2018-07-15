#ifndef SKTYPEDEFS_H
#define SKTYPEDEFS_H

typedef enum {
  SIDE_LEFT = (1<<0),
  SIDE_RIGHT = (1<<1),
  SIDE_TERM = (1<<1)|(1<<0)
} side_t;

typedef struct spoke_s spoke_t;

#endif
