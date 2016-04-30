#include "geometry.h"

#include <stdbool.h>

// Lines intersect, but not necessarily within the line segments
bool intersect( line_t l1, line_t l2, point_t *p )
{
  float k1, k2, m1, m2;

  float y1 = l1.p2.y - l1.p1.y;
  float y2 = l2.p2.y - l2.p1.y;

  float x1 = l1.p2.x - l1.p1.x;
  float x2 = l2.p2.x - l2.p1.x;

  if( x1 == 0 && x2 == 0 ) {
    // Both lines are vertical, no intersection
    return false;
  } else if( x1 == 0 ) {
    // Has to intersect on l1
    p->x = l1.p1.x;

    k2 = y2 / x2;
    m2 = l2.p1.y - k2 * l2.p1.x;

    p->y = k2 * p->x + m2;
    return true;
  } else if( x2 == 0 ) {
    // Has to intersect on l2
    p->x = l2.p2.x;

    k1 = y1 / x1;
    m1 = l1.p1.y - k1 * l1.p1.x;

    p->y = k1 * p->x + m1;
    return true;
  }

  // Generic calculation
  k1 = y1 / x1;
  k2 = y2 / x2;

  // Parallel lines
  if( k1 == k2 ) {
    return false;
  }

  m1 = l1.p1.y - k1 * l1.p1.x;
  m2 = l2.p1.y - k2 * l2.p1.x;

  p->x = (m1 - m2) / (k2 - k1);
  p->y = k1 * p->x + m1;
 
  return true;
}

// True if point is within segment, point must be on
//  the line segment or results will be undefined
bool withinSegment( point_t p, line_t l )
{
  if( l.p1.x != l.p2.x ) {
    float lowX = min( l.p1.x, l.p2.x );
    float highX = max( l.p1.x, l.p2.x );

    if( p.x >= lowX && p.x <= highX )
      return true;
  } else {
    float lowY = min( l.p1.y, l.p2.y );
    float highY = max( l.p1.y, l.p2.y );
    if( p.y >= lowY && p.y <= highY )
      return true;
  }

  return false;
}

bool intersectSegment( line_t l1, line_t l2, point_t *p )
{
  point_t tmpP;
  if( intersect( l1, l2, &tmpP ) ) {
    if( withinSegment( tmpP, l1 ) &&
	withinSegment( tmpP, l2 ) ) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}
