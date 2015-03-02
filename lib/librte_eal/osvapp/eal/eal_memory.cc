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
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <inttypes.h>
#include <fcntl.h>

#include <rte_eal.h>
#include <rte_eal_memconfig.h>
#include <rte_log.h>
#include <rte_string_fns.h>
#include "eal_private.h"
#include "eal_internal_cfg.h"
#include "eal_filesystem.h"

#include <osv/contiguous_alloc.hh>
#include <osv/virt_to_phys.hh>

/*
 * Get physical address of any mapped virtual address in the current process.
 */
phys_addr_t
rte_mem_virt2phy(const void *virtaddr)
{
	/* XXX not implemented. This function is only used by
	 * rte_mempool_virt2phy() when hugepages are disabled. */
	(void)virtaddr;
	return RTE_BAD_PHYS_ADDR;
}

static int
rte_eal_hugepage_init(void)
{
	struct rte_mem_config *mcfg;
	uint64_t total_mem = 0;
	void *addr;
	unsigned i, j, seg_idx = 0;

	/* get pointer to global configuration */
	mcfg = rte_eal_get_configuration()->mem_config;

	/* for debug purposes, hugetlbfs can be disabled */
	if (internal_config.no_hugetlbfs) {
		addr = malloc(internal_config.memory);
		mcfg->memseg[0].phys_addr = (phys_addr_t)(uintptr_t)addr;
		mcfg->memseg[0].addr = addr;
		mcfg->memseg[0].len = internal_config.memory;
		mcfg->memseg[0].socket_id = 0;
		return 0;
	}

	/* map all hugepages and sort them */
	for (i = 0; i < internal_config.num_hugepage_sizes; i ++){
		struct hugepage_info *hpi;
		size_t alloc_size = 0;

		hpi = &internal_config.hugepage_info[i];
		for (j = 0; j < hpi->num_pages[0]; j++) {
			struct rte_memseg *seg;
			uint64_t physaddr;
	
			addr = memory::alloc_phys_contiguous_aligned(hpi->hugepage_sz, hpi->hugepage_sz, false);
			if (addr == nullptr)
				break;
			seg = &mcfg->memseg[seg_idx++];
			seg->addr = addr;
			seg->phys_addr = mmu::virt_to_phys(addr);
			seg->hugepage_sz = hpi->hugepage_sz;
			seg->len = hpi->hugepage_sz;
			seg->nchannel = mcfg->nchannel;
			seg->nrank = mcfg->nrank;
			seg->socket_id = 0;
	
			RTE_LOG(INFO, EAL, "Mapped memory segment %u @ %p: physaddr:0x%"
				PRIx64", len %zu\n",
				0, seg->addr, seg->phys_addr, seg->len);
			if (total_mem >= internal_config.memory ||
					seg_idx >= RTE_MAX_MEMSEG)
				break;
		}
	}
	return 0;
}

static int
rte_eal_memdevice_init(void)
{
	struct rte_config *config;

	if (rte_eal_process_type() == RTE_PROC_SECONDARY)
		return 0;

	config = rte_eal_get_configuration();
	config->mem_config->nchannel = internal_config.force_nchannel;
	config->mem_config->nrank = internal_config.force_nrank;

	return 0;
}

/* init memory subsystem */
int
rte_eal_memory_init(void)
{
	RTE_LOG(INFO, EAL, "Setting up memory...\n");
	const int retval = rte_eal_hugepage_init();
	if (retval < 0)
		return -1;

	if (internal_config.no_shconf == 0 && rte_eal_memdevice_init() < 0)
		return -1;

	return 0;
}
