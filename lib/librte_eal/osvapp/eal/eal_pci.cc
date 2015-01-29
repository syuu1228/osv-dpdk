/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

#include <rte_interrupts.h>
#include <rte_log.h>
#include <rte_pci.h>
#include <rte_common.h>
#include <rte_launch.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_string_fns.h>
#include <rte_debug.h>
#include <rte_devargs.h>

#include "rte_pci_dev_ids.h"
#include "eal_filesystem.h"
#include "eal_private.h"

#include <drivers/device.hh>
#include <drivers/pci-device.hh>

/**
 * @file
 * PCI probing under linux
 *
 * This code is used to simulate a PCI probe by parsing information in
 * sysfs. Moreover, when a registered driver matches a device, the
 * kernel driver currently using it is unloaded and replaced by
 * igb_uio module, which is a very minimal userland driver for Intel
 * network card, only providing access to PCI BAR to applications, and
 * enabling bus master.
 */

struct uio_map {
	void *addr;
	uint64_t offset;
	uint64_t size;
	uint64_t phaddr;
};

/*
 * For multi-process we need to reproduce all PCI mappings in secondary
 * processes, so save them in a tailq.
 */
struct uio_resource {
	TAILQ_ENTRY(uio_resource) next;

	struct rte_pci_addr pci_addr;
	char path[PATH_MAX];
	size_t nb_maps;
	struct uio_map maps[PCI_MAX_RESOURCE];
};

TAILQ_HEAD(uio_res_list, uio_resource);

static struct uio_res_list *uio_res_list = NULL;

static struct rte_tailq_elem rte_pci_tailq = {
	NULL,
	{ NULL, NULL, },
	"PCI_RESOURCE_LIST",
};
EAL_REGISTER_TAILQ(rte_pci_tailq)

/* unbind kernel driver for this device */
static int
pci_unbind_kernel_driver(struct rte_pci_device *dev __rte_unused)
{
	RTE_LOG(ERR, EAL, "RTE_PCI_DRV_FORCE_UNBIND flag is not implemented "
		"for OSv\n");
	return -ENOTSUP;
}

/* Scan one pci entry, and fill the devices list from it. */
static int
pci_scan_one(hw::hw_device* dev)
{
	u8 bus, device, func;
	auto pci_dev = static_cast<pci::device*>(dev);
	auto rte_dev = new rte_pci_device();

	/* get bus id, device id, function no */
	pci_dev->get_bdf(bus, device, func);
	rte_dev->addr.domain = 0;
	rte_dev->addr.bus = bus;
	rte_dev->addr.devid = device;
	rte_dev->addr.function = func;

	/* get vendor id */
	rte_dev->id.vendor_id = pci_dev->get_vendor_id();

	/* get device id */
	rte_dev->id.device_id = pci_dev->get_device_id();

	/* get subsystem_vendor id */
	rte_dev->id.subsystem_vendor_id = pci_dev->get_subsystem_vid();

	/* get subsystem_device id */
	rte_dev->id.subsystem_device_id = pci_dev->get_subsystem_id();

	/* TODO: get max_vfs */
	rte_dev->max_vfs = 0;

	/* OSv has no NUMA support (yet) */
	rte_dev->numa_node = -1;

	/* Disable interrupt */
	rte_dev->intr_handle.fd = -1;
	rte_dev->intr_handle.type = RTE_INTR_HANDLE_UNKNOWN;

	for (int i = 0; ; i++) {
		auto bar = pci_dev->get_bar(i+1);
		if (bar == nullptr) {
			RTE_LOG(DEBUG, EAL, "   bar%d not available\n", i);
			break;
		}
		if (bar->is_mmio()) {
			rte_dev->mem_resource[i].len = bar->get_size();
			rte_dev->mem_resource[i].phys_addr = bar->get_addr64();
			bar->map();
			rte_dev->mem_resource[i].addr = const_cast<void *>(bar->get_mmio());
		} else {
			rte_dev->mem_resource[i].len = bar->get_size();
			rte_dev->mem_resource[i].phys_addr = 0;
			rte_dev->mem_resource[i].addr = reinterpret_cast<void *>(bar->get_addr_lo());
		}
	}

	/* device is valid, add in list (sorted) */
	if (TAILQ_EMPTY(&pci_device_list)) {
		TAILQ_INSERT_TAIL(&pci_device_list, rte_dev, next);
	}
	else {
		struct rte_pci_device *dev2 = NULL;
		int ret;

		TAILQ_FOREACH(dev2, &pci_device_list, next) {
			ret = rte_eal_compare_pci_addr(&rte_dev->addr, &dev2->addr);
			if (ret > 0)
				continue;
			else if (ret < 0) {
				TAILQ_INSERT_BEFORE(dev2, rte_dev, next);
				return 1;
			} else { /* already registered */
				dev2->kdrv = rte_dev->kdrv;
				dev2->max_vfs = rte_dev->max_vfs;
				memmove(dev2->mem_resource,
					rte_dev->mem_resource,
					sizeof(rte_dev->mem_resource));
				delete rte_dev;
				return 0;
			}
		}
		TAILQ_INSERT_TAIL(&pci_device_list, rte_dev, next);
	}

	return 1;
}

/*
 * Scan the content of the PCI bus, and add the devices in the devices
 * list. Call pci_scan_one() for each pci entry found.
 */
static int
pci_scan(void)
{
	unsigned dev_count = 0;
	int err = 0;

	auto dm = hw::device_manager::instance();
	dm->for_each_device([&dev_count, &err] (hw::hw_device* dev) {
		if (dev->is_attached())
			return;
		int ret = pci_scan_one(dev);
		if (ret < 0) {
			err++;
		} else {
			dev_count += ret;
		}
	});

	if (err)
		return -1;

	RTE_LOG(ERR, EAL, "PCI scan found %u devices\n", dev_count);
	return 0;
}

/*
 * If vendor/device ID match, call the devinit() function of the
 * driver.
 */
int
rte_eal_pci_probe_one_driver(struct rte_pci_driver *dr, struct rte_pci_device *dev)
{
	struct rte_pci_id *id_table;
	int ret;

	for (id_table = dr->id_table ; id_table->vendor_id != 0; id_table++) {

		/* check if device's identifiers match the driver's ones */
		if (id_table->vendor_id != dev->id.vendor_id &&
				id_table->vendor_id != PCI_ANY_ID)
			continue;
		if (id_table->device_id != dev->id.device_id &&
				id_table->device_id != PCI_ANY_ID)
			continue;
		if (id_table->subsystem_vendor_id != dev->id.subsystem_vendor_id &&
				id_table->subsystem_vendor_id != PCI_ANY_ID)
			continue;
		if (id_table->subsystem_device_id != dev->id.subsystem_device_id &&
				id_table->subsystem_device_id != PCI_ANY_ID)
			continue;

		struct rte_pci_addr *loc = &dev->addr;

		RTE_LOG(DEBUG, EAL, "PCI device " PCI_PRI_FMT " on NUMA socket %i\n",
				loc->domain, loc->bus, loc->devid, loc->function,
				dev->numa_node);

		RTE_LOG(DEBUG, EAL, "  probe driver: %x:%x %s\n", dev->id.vendor_id,
				dev->id.device_id, dr->name);

		/* no initialization when blacklisted, return without error */
		if (dev->devargs != NULL &&
			dev->devargs->type == RTE_DEVTYPE_BLACKLISTED_PCI) {

			RTE_LOG(DEBUG, EAL, "  Device is blacklisted, not initializing\n");
			return 0;
		}

		if (dr->drv_flags & RTE_PCI_DRV_FORCE_UNBIND &&
		           rte_eal_process_type() == RTE_PROC_PRIMARY) {
			/* unbind current driver */
			if (pci_unbind_kernel_driver(dev) < 0)
				return -1;
		}

		/* reference driver structure */
		dev->driver = dr;

		/* call the driver devinit() function */
		return dr->devinit(dr, dev);
	}
	/* return positive value if driver is not found */
	return 1;
}

/* Init the PCI EAL subsystem */
int
rte_eal_pci_init(void)
{
	TAILQ_INIT(&pci_driver_list);
	TAILQ_INIT(&pci_device_list);
	uio_res_list = RTE_TAILQ_CAST(rte_pci_tailq.head, uio_res_list);

	/* for debug purposes, PCI can be disabled */
	if (internal_config.no_pci)
		return 0;

	if (pci_scan() < 0) {
		RTE_LOG(ERR, EAL, "%s(): Cannot scan PCI bus\n", __func__);
		return -1;
	}
	return 0;
}
