#ifndef VECTOR_H
#define VECTOR_H

/* $Id$ */

typedef struct { double l, b; } pVector;
typedef double cVector[3];
typedef double rotMatrix[3][3];

double norm(cVector v);
void normalise(cVector v);
double dot(cVector u, cVector v);
void cross(cVector a, cVector b, cVector c);
double distance(cVector u, cVector v);

#endif
