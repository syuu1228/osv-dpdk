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

#include <rte_log.h>
#include <rte_common.h>
#include "eal_internal_cfg.h"
#include "eal_hugepages.h"

#define CONTIGUOUS_CHUNK_SIZE RTE_STR(RTE_CONTIGUOUS_CHUNK_SIZE)

static inline size_t
get_contiguous_chunk_size(void)
{
	return rte_str_to_size(CONTIGUOUS_CHUNK_SIZE);
}

int
eal_hugepage_info_init(void)
{
	struct hugepage_info *hpi = &internal_config.hugepage_info[0];

	internal_config.num_hugepage_sizes = 1;
	/* size of contiguous chunk size for eal_memory.cc */
	hpi->hugepage_sz = get_contiguous_chunk_size();
	/* this will ignored */
	hpi->hugedir = "/";
	/* only used when -m is not specified */
	hpi->num_pages[0] = RTE_DEFAULT_NUM_CHUNKS;
	/* this will ignored */
	hpi->lock_descriptor = -1;

	return 0;
}
