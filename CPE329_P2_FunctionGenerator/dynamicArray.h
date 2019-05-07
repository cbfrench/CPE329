/*
 * dynamicArray.h
 *
 *  Created on: Apr 21, 2019
 *      Author: Connor French and Joe Sandoval
 *
 *  Implements Dynamically-Resizable Arrays in C using Malloc
 */
#ifndef DYN_ARRAY
#define DYN_ARRAY

#include <stddef.h>

typedef struct {
  int *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize);
void insertArray(Array *a, int element);
void freeArray(Array *a);

#endif
