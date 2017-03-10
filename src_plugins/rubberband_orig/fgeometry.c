
#include "fgeometry.h"
#include <math.h>
#include <stdio.h>

#define MIN_COMPONENT	0.000001

int pcb_fvector_is_null (pcb_fvector_t v)
{
	return fabs(v.x) < MIN_COMPONENT && fabs(v.y) < MIN_COMPONENT;
}

double pcb_fvector_dot (pcb_fvector_t v1, pcb_fvector_t v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}


void pcb_fvector_normalize (pcb_fvector_t *v)
{
	double module = sqrt(v->x * v->x + v->y * v->y);
	
	v->x /= module;
	v->y /= module;
}

int pcb_fline_is_valid(pcb_fline_t l)
{
	return !pcb_fvector_is_null(l.direction);
}

pcb_fline_t pcb_fline_create(pcb_any_line_t *line)
{
	pcb_fline_t ret;
	
	ret.point.x = line->Point1.X;
	ret.point.y = line->Point1.Y;
	ret.direction.x = line->Point2.X - line->Point1.X;
	ret.direction.y = line->Point2.Y - line->Point1.Y;
	
	if (!pcb_fvector_is_null (ret.direction))
		pcb_fvector_normalize( &ret.direction );
	
	return ret;
}


pcb_fline_t pcb_fline_create_from_points(struct pcb_point_s *base_point, struct pcb_point_s *other_point)
{
	pcb_fline_t ret;
	
	ret.point.x = base_point->X;
	ret.point.y = base_point->Y;
	ret.direction.x = other_point->X - base_point->X;
	ret.direction.y = other_point->Y - base_point->Y;
	
	if (!pcb_fvector_is_null (ret.direction))
		pcb_fvector_normalize( &ret.direction );
	
	return ret;
}


pcb_fvector_t pcb_fline_intersection(pcb_fline_t l1, pcb_fline_t l2)
{
	pcb_fvector_t ret;
	double lines_dot;

	ret.x = 0;
	ret.y = 0;

	lines_dot = pcb_fvector_dot (l1.direction, l2.direction);
	if (fabs(lines_dot) > 0.990 )
	{
		/* Consider them parallel. Return null point (vector) */
		return ret;
	}

	{
	/*
	 * *** From polygon1.c ***
	 * We have the lines:
	 * l1: p1 + s*d1
	 * l2: p2 + t*d2
	 * And we want to know the intersection point.
	 * Calculate t:
	 * p1 + s*d1 = p2 + t*d2
	 * which is similar to the two equations:
	 * p1x + s * d1x = p2x + t * d2x
	 * p1y + s * d1y = p2y + t * d2y
	 * Multiplying these by d1y resp. d1x gives:
	 * d1y * p1x + s * d1x * d1y = d1y * p2x + t * d1y * d2x
	 * d1x * p1y + s * d1x * d1y = d1x * p2y + t * d1x * d2y
	 * Subtracting these gives:
	 * d1y * p1x - d1x * p1y = d1y * p2x - d1x * p2y + t * ( d1y * d2x - d1x * d2y )
	 * So t can be isolated:
	 * t = (d1y * ( p1x - p2x ) + d1x * ( - p1y + p2y )) / ( d1y * d2x - d1x * d2y )
	 * and s can be found similarly
	 * s = (d2y * ( p2x - p1x ) + d2x * ( p1y - p2y )) / ( d2y * d1x - d2x * d1y)
	 */
	
	double t;
	double p1x, p1y, d1x, d1y;
	double p2x, p2y, d2x, d2y;
	
	p1x = l1.point.x;
	p1y = l1.point.y;
	d1x = l1.direction.x;
	d1y = l1.direction.y;
	
	p2x = l2.point.x;
	p2y = l2.point.y;
	d2x = l2.direction.x;
	d2y = l2.direction.y;
	
	t = (d1y * ( p1x - p2x ) + d1x * ( - p1y + p2y )) / ( d1y * d2x - d1x * d2y );
	s = (d2y * ( p2x - p1x ) + d2x * ( p1y - p2y )) / ( d2y * d1x - d2x * d1y );

	/*printf("Intersection t=%f, s=%f\n", t, s); */
	
/*	ret.x = p1x + s * d1x;
	ret.y = p1y + s * d1y;*/
	
	ret.x = p2x + t * d2x;
	ret.y = p2y + t * d2y;
	
	
	/*printf("Intersection x=%f, y=%f\n", ret.x, ret.y);*/
	}
	return ret;
}


