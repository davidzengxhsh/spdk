#ifndef __IOAT_IMPL_H__
#define __IOAT_IMPL_H__

#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <rte_config.h>
#include <rte_malloc.h>
#include <rte_atomic.h>
#include <rte_cycles.h>

#include "spdk/assert.h"
#include "spdk/pci.h"
#include "spdk/vtophys.h"
#include "spdk/pci.h"

#include "ioat_pci.h"

#ifdef SPDK_CONFIG_PCIACCESS
#include <pciaccess.h>
#else
#include <rte_pci.h>
#endif

/**
 * \file
 *
 * This file describes the functions required to integrate
 * the userspace IOAT driver for a specific implementation.  This
 * implementation is specific for DPDK.  Users would revise it as
 * necessary for their own particular environment if not using it
 * within the SPDK framework.
 */

/**
 * Allocate a pinned, physically contiguous memory buffer with the
 * given size and alignment.
 */
static inline void *
ioat_zmalloc(const char *tag, size_t size, unsigned align, uint64_t *phys_addr)
{
	void *buf = rte_malloc(tag, size, align);

	if (buf) {
		memset(buf, 0, size);
		*phys_addr = rte_malloc_virt2phy(buf);
	}
	return buf;
}

/**
 * Free a memory buffer previously allocated with ioat_zmalloc.
 */
#define ioat_free(buf)			rte_free(buf)

/**
 * Return the physical address for the specified virtual address.
 */
#define ioat_vtophys(buf)		spdk_vtophys(buf)

/**
 * Delay us.
 */
#define ioat_delay_us(us)        rte_delay_us(us)

/**
 * Log or print a message from the driver.
 */
#define ioat_printf(chan, fmt, args...) printf(fmt, ##args)

/**
 *
 */
#define ioat_pcicfg_read32(handle, var, offset)  spdk_pci_device_cfg_read32(handle, var, offset)
#define ioat_pcicfg_write32(handle, var, offset) spdk_pci_device_cfg_write32(handle, var, offset)

struct ioat_pci_enum_ctx {
	int (*user_enum_cb)(void *enum_ctx, struct spdk_pci_device *pci_dev);
	void *user_enum_ctx;
};

#ifdef SPDK_CONFIG_PCIACCESS

static inline bool
ioat_pci_device_match_id(uint16_t vendor_id, uint16_t device_id)
{
	if (vendor_id != SPDK_PCI_VID_INTEL) {
		return false;
	}

	switch (device_id) {
	case PCI_DEVICE_ID_INTEL_IOAT_SNB0:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB1:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB2:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB3:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB4:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB5:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB6:
	case PCI_DEVICE_ID_INTEL_IOAT_SNB7:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB0:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB1:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB2:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB3:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB4:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB5:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB6:
	case PCI_DEVICE_ID_INTEL_IOAT_IVB7:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW0:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW1:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW2:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW3:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW4:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW5:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW6:
	case PCI_DEVICE_ID_INTEL_IOAT_HSW7:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX0:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX1:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX2:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX3:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX4:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX5:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX6:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX7:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX8:
	case PCI_DEVICE_ID_INTEL_IOAT_BDX9:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD0:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD1:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD2:
	case PCI_DEVICE_ID_INTEL_IOAT_BWD3:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE0:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE1:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE2:
	case PCI_DEVICE_ID_INTEL_IOAT_BDXDE3:
		return true;
	}

	return false;
}

static int
ioat_pci_enum_cb(void *enum_ctx, struct spdk_pci_device *pci_dev)
{
	struct ioat_pci_enum_ctx *ctx = enum_ctx;
	uint16_t vendor_id = spdk_pci_device_get_vendor_id(pci_dev);
	uint16_t device_id = spdk_pci_device_get_device_id(pci_dev);

	if (!ioat_pci_device_match_id(vendor_id, device_id)) {
		return 0;
	}

	return ctx->user_enum_cb(ctx->user_enum_ctx, pci_dev);
}

static inline int
ioat_pci_enumerate(int (*enum_cb)(void *enum_ctx, struct spdk_pci_device *pci_dev), void *enum_ctx)
{
	struct ioat_pci_enum_ctx ioat_enum_ctx;

	ioat_enum_ctx.user_enum_cb = enum_cb;
	ioat_enum_ctx.user_enum_ctx = enum_ctx;

	return spdk_pci_enumerate(ioat_pci_enum_cb, &ioat_enum_ctx);
}

static inline int
ioat_pcicfg_map_bar(void *devhandle, uint32_t bar, uint32_t read_only, void **mapped_addr)
{
	struct pci_device *dev = devhandle;
	uint32_t flags = (read_only ? 0 : PCI_DEV_MAP_FLAG_WRITABLE);

	return pci_device_map_range(dev, dev->regions[bar].base_addr, 4096,
				    flags, mapped_addr);
}

static inline int
ioat_pcicfg_unmap_bar(void *devhandle, uint32_t bar, void *addr)
{
	struct pci_device *dev = devhandle;

	return pci_device_unmap_range(dev, addr, dev->regions[bar].size);
}

#else /* !SPDK_CONFIG_PCIACCESS */

static inline int
ioat_pcicfg_map_bar(void *devhandle, uint32_t bar, uint32_t read_only, void **mapped_addr)
{
	struct rte_pci_device *dev = devhandle;

	*mapped_addr = dev->mem_resource[bar].addr;
	return 0;
}

static inline int
ioat_pcicfg_unmap_bar(void *devhandle, uint32_t bar, void *addr)
{
	return 0;
}

#define SPDK_IOAT_PCI_DEVICE(DEVICE_ID) RTE_PCI_DEVICE(SPDK_PCI_VID_INTEL, DEVICE_ID)

/* TODO: avoid duplicating the device ID list */
static struct rte_pci_id ioat_driver_id[] = {
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB1)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB4)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB5)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB6)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB7)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_SNB8)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB1)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB4)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB5)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB6)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB7)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB8)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_IVB9)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW4)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW5)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW6)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW7)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW8)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_HSW9)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BWD0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BWD1)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BWD2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BWD3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDXDE0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDXDE1)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDXDE2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDXDE3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX0)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX1)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX2)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX3)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX4)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX5)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX6)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX7)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX8)},
	{SPDK_IOAT_PCI_DEVICE(PCI_DEVICE_ID_INTEL_IOAT_BDX9)},
	{ .vendor_id = 0, /* sentinel */ },
};

/*
 * TODO: eliminate this global if possible (does rte_pci_driver have a context field for this?)
 *
 * This should be protected by the ioat driver lock, since ioat_probe() holds the lock
 *  the whole time, but we shouldn't have to depend on that.
 */
static struct ioat_pci_enum_ctx g_ioat_pci_enum_ctx;

static int
ioat_driver_init(struct rte_pci_driver *dr, struct rte_pci_device *rte_dev)
{
	/*
	 * These are actually the same type internally.
	 * TODO: refactor this so it's inside pci.c
	 */
	struct spdk_pci_device *pci_dev = (struct spdk_pci_device *)rte_dev;

	return g_ioat_pci_enum_ctx.user_enum_cb(g_ioat_pci_enum_ctx.user_enum_ctx, pci_dev);
}

static struct rte_pci_driver ioat_rte_driver = {
	.name = "ioat_driver",
	.devinit = ioat_driver_init,
	.id_table = ioat_driver_id,
	.drv_flags = RTE_PCI_DRV_NEED_MAPPING,
};

static inline int
ioat_pci_enumerate(int (*enum_cb)(void *enum_ctx, struct spdk_pci_device *pci_dev), void *enum_ctx)
{
	int rc;

	g_ioat_pci_enum_ctx.user_enum_cb = enum_cb;
	g_ioat_pci_enum_ctx.user_enum_ctx = enum_ctx;

	rte_eal_pci_register(&ioat_rte_driver);
	rc = rte_eal_pci_probe();
	rte_eal_pci_unregister(&ioat_rte_driver);

	return rc;
}

#endif /* !SPDK_CONFIG_PCIACCESS */

#endif /* __IOAT_IMPL_H__ */
