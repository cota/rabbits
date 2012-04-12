#ifndef _IO_H_
#define _IO_H_

#include <stdlib.h>

static inline uint32_t ioread32(const volatile void *addr)
{
	return *(const volatile uint32_t *)addr;
}

static inline void iowrite32(uint32_t b, volatile void *addr)
{
	*(volatile uint32_t *)addr = b;
}

#endif /* _IO_H_ */
