#include "compat.h"
#include <stdio.h>

int __errno;

// This is a stub to resolve a linker error from Box2D.
//
// Though Box2D allows to set custom memory management
// functions via `b2SetAllocator`, there's still a linkage error.
// This stub should never be called.
void* aligned_alloc(unsigned int size, int alignment)
{
	(void)size;
	(void)alignment;
	printf("aligned_alloc() is not supported");
	return NULL;
}

void* custom_aligned_alloc(unsigned int size, int alignment)
{
	if (size == 0) {
		return NULL;
	}

	uintptr_t align_uint = (uintptr_t)alignment;

	void* original_ptr = malloc(size + align_uint - 1 + sizeof(void*));
	if (original_ptr == NULL) {
		printf("out of memory\n");
		return NULL;
	}

	void* ptr_storage = (void*)((uintptr_t)original_ptr + sizeof(void*));
	void* aligned_ptr = (void*)(((uintptr_t)ptr_storage + align_uint - 1) & ~(align_uint - 1));

	((void**)aligned_ptr)[-1] = original_ptr;

	return aligned_ptr;
}

void custom_aligned_free(void* ptr)
{
	if (ptr == NULL) {
		return;
	}

	// the pointer is stored before the aligned block
	void* original_ptr = ((void**)ptr)[-1];
	free(original_ptr);
}

// This is NOT an atomic operation, it is only safe in a single-threaded environment.
uint32_t __atomic_fetch_add_4(uint32_t* ptr, uint32_t val, int memorder)
{
	(void)memorder;
	uint32_t old_val = *ptr;
	*ptr = old_val + val;
	return old_val;
}

// This is NOT an atomic operation, it is only safe in a single-threaded environment.
bool __atomic_compare_exchange_4(uint32_t* ptr, uint32_t* expected, uint32_t desired, int success_memorder, int failure_memorder)
{
	(void)success_memorder;
	(void)failure_memorder;
	if (*ptr == *expected) {
		*ptr = desired;
		return true;
	} else {
		*expected = *ptr;
		return false;
	}
}
