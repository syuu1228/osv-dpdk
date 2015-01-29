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

#include <string.h>
#include <dirent.h>
#include <sys/mman.h>

#include <rte_log.h>
#include <rte_pci.h>
#include <rte_tailq.h>
#include <rte_eal_memconfig.h>
#include <rte_malloc.h>
#include <rte_devargs.h>

#include "rte_pci_dev_ids.h"
#include "eal_filesystem.h"
#include "eal_private.h"

#include "drivers/device.hh"
#include "drivers/pci-device.hh"

/**
 * @file
 * PCI probing under linux
 *
 * This code is used to simulate a PCI probe by parsing information in sysfs.
 * When a registered device matches a driver, it is then initialized with
 * IGB_UIO driver (or doesn't initialize, if the device wasn't bound to it).
 */

struct pci_map {
	void *addr;
	uint64_t offset;
	uint64_t size;
	uint64_t phaddr;
};

/*
 * For multi-process we need to reproduce all PCI mappings in secondary
 * processes, so save them in a tailq.
 */
struct mapped_pci_resource {
	TAILQ_ENTRY(mapped_pci_resource) next;

	struct rte_pci_addr pci_addr;
	int nb_maps;
	struct pci_map maps[PCI_MAX_RESOURCE];
};

TAILQ_HEAD(mapped_pci_res_list, mapped_pci_resource);

static struct mapped_pci_res_list *pci_res_list = NULL;

/* unbind kernel driver for this device */
static int
pci_unbind_kernel_driver(struct rte_pci_device *rte_dev)
{
	return -1; /* XXX: implement it */
}

/* Compare two PCI device addresses. */
static int
pci_addr_comparison(struct rte_pci_addr *addr, struct rte_pci_addr *addr2)
{
	uint64_t dev_addr = (addr->domain << 24) + (addr->bus << 16) + (addr->devid << 8) + addr->function;
	uint64_t dev_addr2 = (addr2->domain << 24) + (addr2->bus << 16) + (addr2->devid << 8) + addr2->function;

	if (dev_addr > dev_addr2)
		return 1;
	else
		return 0;
}

/*
 * If vendor/device ID match, call the devinit() function of the
 * driver.
 */
int
rte_eal_pci_probe_one_driver(struct rte_pci_driver *dr, struct rte_pci_device *dev)
{
	int ret;
	struct rte_pci_id *id_table;

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
			return 1;
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
	pci_res_list = RTE_TAILQ_RESERVE_BY_IDX(RTE_TAILQ_PCI,
			mapped_pci_res_list);

	/* for debug purposes, PCI can be disabled */
	if (internal_config.no_pci)
		return 0;

	auto dm = hw::device_manager::instance();
	dm->for_each_device([] (hw::hw_device* dev) {
		u8 bus, device, func;
		auto pci_dev = static_cast<pci::device*>(dev);
		auto rte_dev = new rte_pci_device();
		pci_dev->get_bdf(bus, device, func);
		rte_dev->addr.domain = 0;
		rte_dev->addr.bus = bus;
		rte_dev->addr.devid = device;
		rte_dev->addr.function = func;
		rte_dev->id.vendor_id = pci_dev->get_vendor_id();
		rte_dev->id.device_id = pci_dev->get_device_id();
		rte_dev->id.subsystem_vendor_id = pci_dev->get_subsystem_vid();
		rte_dev->id.subsystem_device_id = pci_dev->get_subsystem_id();
		rte_dev->max_vfs = 0;
		rte_dev->numa_node = -1;

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
			struct rte_pci_device *rte_dev2 = NULL;
	
			TAILQ_FOREACH(rte_dev2, &pci_device_list, next) {
				if (pci_addr_comparison(&rte_dev->addr, &rte_dev2->addr))
					continue;
				else {
					TAILQ_INSERT_BEFORE(rte_dev2, rte_dev, next);
					return 0;
				}
			}
			TAILQ_INSERT_TAIL(&pci_device_list, rte_dev, next);
		}
	});
	return 0;
}
