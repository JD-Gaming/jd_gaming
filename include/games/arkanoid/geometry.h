#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdbool.h>

#ifndef min
#  define min(__a__, __b__) ((__a__) < (__b__) ? (__a__) : (__b__))
#endif
#ifndef max
#  define max(__a__, __b__) ((__a__) > (__b__) ? (__a__) : (__b__))
#endif

typedef struct point_s {
	float x;
	float y;
} point_t;

typedef struct line_s {
	point_t p1;
	point_t p2;
} line_t;

// True if lines intersect, even if not within the line segments
bool intersect( line_t l1, line_t l2, point_t *p );

// True only if lines intersect within line segments
bool intersectSegment( line_t l1, line_t l2, point_t *p );

// True if point is within segment, point must be on
//  the line segment or results will be undefined
bool withinSegment( point_t p, line_t l );

#endif
