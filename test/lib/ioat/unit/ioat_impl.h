/* Unit test stubbed version of ioat_impl.h */

#ifndef __IOAT_IMPL_H__
#define __IOAT_IMPL_H__

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct spdk_pci_device;

static inline void *
ioat_zmalloc(const char *tag, size_t size, unsigned align, uint64_t *phys_addr)
{
	return calloc(1, size);
}

#define ioat_noop()					do { } while (0)

#define ioat_calloc(tag, num, size, align)		calloc(num, size)
#define ioat_malloc(tag, size, align)			malloc(size)
#define ioat_free(buf)					free(buf)
#define ioat_vtophys(buf)				(uint64_t)(buf)
#define ioat_delay_us(us)				ioat_noop()
#define ioat_printf(chan, fmt, args...)			printf(fmt, ##args)

static inline int
ioat_pci_enumerate(int (*enum_cb)(void *enum_ctx, struct spdk_pci_device *pci_dev), void *enum_ctx)
{
	return -1;
}

#define ioat_pcicfg_read32(handle, var, offset)		do { *(var) = 0xFFFFFFFFu; } while (0)
#define ioat_pcicfg_write32(handle, var, offset)	do { (void)(var); } while (0)

static inline int
ioat_pcicfg_map_bar(void *devhandle, uint32_t bar, uint32_t read_only, void **mapped_addr)
{
	/* TODO */
	*mapped_addr = NULL;
	return -1;
}

static inline int
ioat_pcicfg_unmap_bar(void *devhandle, uint32_t bar, void *addr)
{
	return 0;
}

#endif /* __IOAT_IMPL_H__ */
