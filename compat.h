#ifndef COMPAT_H
#define COMPAT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

void* aligned_alloc(unsigned int size, int alignment);
void* custom_aligned_alloc(size_t size, int alignment);
void custom_aligned_free(void* mem);

uint32_t __atomic_fetch_add_4(uint32_t* ptr, uint32_t val, int memorder);
bool __atomic_compare_exchange_4(uint32_t* ptr, uint32_t* expected, uint32_t desired, int success_memorder, int failure_memorder);

// these functions are hidden by fpdoom/fpdoom/include/math.h
float sqrtf(float);
float remainderf(float, float);
float sinf(float);
float cosf(float);
float fmaxf(float, float);
float fminf(float, float);
float floorf(float);
float ceilf(float);
int isnan(double);
int isinf(double);

#endif
