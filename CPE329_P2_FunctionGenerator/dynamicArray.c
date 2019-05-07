/*
 * dynamicArray.c
 *
 *  Created on: Apr 21, 2019
 *      Author: Connor French and Joe Sandoval
 *
 *  Implements Dynamically-Resizable Arrays in C using Malloc
 */
#include "dynamicArray.h"

void initArray(Array *a, size_t initialSize) {
  // Initializes Array at address 'a'
  a->array = (int *)malloc(initialSize * sizeof(int));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, int element) {
  // Inserts 'element' into array at address 'a', re-allocating space as needed
  if (a->used == a->size) {
    a->size *= 2;
    a->array = (int *)realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  // Frees memory used by array at address 'a'
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}
