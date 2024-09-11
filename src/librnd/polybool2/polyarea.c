/* include all files of the implementation (so inlining works without depending
   on link time optimization */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <librnd/rnd_config.h>
#include <librnd/core/math_helper.h>
#include <librnd/core/heap.h>
#include <librnd/core/rnd_printf.h>


#include "polyconf.h"

#include "polyarea.h"
#include "rtree.h"
#include "rtree2_compat.h"
#include "pa.h"
#include "pa_prints.h"

#include "pa_math.c"
#include "pa_vect.c"
#include "pa_debug.c"
#include "pa_pline.c"
#include "pa_polyarea.c"

#include "pa_segment.c"
#include "pa_dicer.c"

#include "pa_api_bool.c"
#include "pa_api_vertex.c"
#include "pa_api_pline.c"
#include "pa_api_polyarea.c"
#include "pa_api_check.c"
#include "pa_api_misc.c"

#include "pa_compat.c"

int rnd_polybool_assert_on_error_code = 0;
