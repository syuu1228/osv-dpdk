#   BSD LICENSE
#
#   Copyright 2012-2015 6WIND S.A.
#   Copyright 2012 Mellanox.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of 6WIND S.A. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include $(RTE_SDK)/mk/rte.vars.mk

# Library name.
LIB = librte_pmd_mlx4.a

# Sources.
SRCS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += mlx4.c

# Dependencies.
DEPDIRS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += lib/librte_ether
DEPDIRS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += lib/librte_mbuf
DEPDIRS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += lib/librte_eal
DEPDIRS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += lib/librte_mempool
DEPDIRS-$(CONFIG_RTE_LIBRTE_MLX4_PMD) += lib/librte_malloc

# Basic CFLAGS.
CFLAGS += -O3
CFLAGS += -std=gnu99 -Wall -Wextra
CFLAGS += -g
CFLAGS += -I.
CFLAGS += -D_XOPEN_SOURCE=600
CFLAGS += $(WERROR_FLAGS)

# A few warnings cannot be avoided in external headers.
CFLAGS += -Wno-error=cast-qual

EXPORT_MAP := rte_pmd_mlx4_version.map
LIBABIVER := 1

# DEBUG which is usually provided on the command-line may enable
# CONFIG_RTE_LIBRTE_MLX4_DEBUG.
ifeq ($(DEBUG),1)
CONFIG_RTE_LIBRTE_MLX4_DEBUG := y
endif

# User-defined CFLAGS.
ifeq ($(CONFIG_RTE_LIBRTE_MLX4_DEBUG),y)
CFLAGS += -pedantic -UNDEBUG -DPEDANTIC
else
CFLAGS += -DNDEBUG -UPEDANTIC
endif

ifdef CONFIG_RTE_LIBRTE_MLX4_SGE_WR_N
CFLAGS += -DMLX4_PMD_SGE_WR_N=$(CONFIG_RTE_LIBRTE_MLX4_SGE_WR_N)
endif

ifdef CONFIG_RTE_LIBRTE_MLX4_MAX_INLINE
CFLAGS += -DMLX4_PMD_MAX_INLINE=$(CONFIG_RTE_LIBRTE_MLX4_MAX_INLINE)
endif

ifdef CONFIG_RTE_LIBRTE_MLX4_TX_MP_CACHE
CFLAGS += -DMLX4_PMD_TX_MP_CACHE=$(CONFIG_RTE_LIBRTE_MLX4_TX_MP_CACHE)
endif

ifdef CONFIG_RTE_LIBRTE_MLX4_SOFT_COUNTERS
CFLAGS += -DMLX4_PMD_SOFT_COUNTERS=$(CONFIG_RTE_LIBRTE_MLX4_SOFT_COUNTERS)
endif

ifdef CONFIG_RTE_LIBRTE_MLX4_COMPAT_VMWARE
CFLAGS += -DMLX4_PMD_COMPAT_VMWARE=$(CONFIG_RTE_LIBRTE_MLX4_COMPAT_VMWARE)
endif

include $(RTE_SDK)/mk/rte.lib.mk

# Generate and clean-up mlx4_autoconf.h.

export CC CFLAGS CPPFLAGS EXTRA_CFLAGS EXTRA_CPPFLAGS
export AUTO_CONFIG_CFLAGS = -Wno-error

mlx4_autoconf.h: $(RTE_SDK)/scripts/auto-config-h.sh
	$Q $(RM) -f -- '$@'
	$Q sh -- '$<' '$@' \
		RSS_SUPPORT \
		infiniband/verbs.h \
		enum IBV_EXP_DEVICE_UD_RSS
	$Q sh -- '$<' '$@' \
		INLINE_RECV \
		infiniband/verbs.h \
		enum IBV_EXP_DEVICE_ATTR_INLINE_RECV_SZ
	$Q sh -- '$<' '$@' \
		SEND_RAW_WR_SUPPORT \
		infiniband/verbs.h \
		type 'struct ibv_send_wr_raw'

mlx4.o: mlx4_autoconf.h

clean_mlx4: FORCE
	$Q rm -f -- mlx4_autoconf.h

clean: clean_mlx4
