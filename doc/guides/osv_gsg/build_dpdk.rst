..  BSD LICENSE
    Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.. _building_from_source:

Compiling the DPDK Target from Source
=====================================

System Requirements
-------------------

To building DPDK for OSv, you will need to use Linux/x86_64 with g++ 4.8 or later.

Install the DPDK and Browse Sources
-----------------------------------

First, uncompress the archive and move to the DPDK source directory:

.. code-block:: console

    [user@host ~]$ unzip DPDK-<version>zip
    [user@host ~]$ cd DPDK-<version>
    [user@host DPDK]$ ls
    app/ config/ examples/ lib/ LICENSE.GPL LICENSE.LGPL Makefile mk/ scripts/ tools/

The DPDK is composed of several directories:

*   lib: Source code of DPDK libraries

*   app: Source code of DPDK applications (automatic tests)

*   examples: Source code of DPDK applications

*   config, tools, scripts, mk: Framework-related makefiles, scripts and configuration

Install Capstan
--------------------------------------------

Before start building VM image, you need to install Capstan*:

`http://osv.io/capstan/`

.. code-block:: console

    [user@host ~]$ curl https://raw.githubusercontent.com/cloudius-systems/capstan/master/scripts/download | bash

Build DPDK for OSv VM image
--------------------------------------------

Build VM image using Capstan:

.. code-block:: console

    [user@host ~]$ cd DPDK-<version>/lib/librte_eal/osvapp/capstan/
    [user@host capstan]$ capstan build osv-dpdk
    Building osv-dpdk...
    Downloading cloudius/osv-base/index.yaml...
    145 B / 145 B [======================================================] 100.00 %
    Downloading cloudius/osv-base/osv-base.qemu.gz...
    20.09 MB / 20.09 MB [================================================] 100.00 %
    Uploading files...
    10 / 10 [============================================================] 100.00 %

Run DPDK for OSv VM image
--------------------------------------------

Run VM image using Capstan:

.. code-block:: console

    [user@host ~]$ capstan run osv-dpdk
    Created instance: osv-dpdk
    OSv v0.19
    eth0: 192.168.122.15
    EAL: Detected lcore 0 as core 0 on socket 0
    EAL: Detected lcore 1 as core 1 on socket 0
    EAL: Support maximum 128 logical core(s) by configuration.
    EAL: Detected 2 lcore(s)
    EAL:    bar2 not available
    EAL:    bar2 not available
    EAL:    bar2 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL: PCI scan found 7 devices
    EAL: Setting up memory...
    EAL: Mapped memory segment 0 @ 0xffff80003de00000: physaddr:0x3de00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80003bc00000: physaddr:0x3bc00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800039a00000: physaddr:0x39a00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800037800000: physaddr:0x37800000, len 33554432
    EAL: TSC frequency is ~438348 KHz
    EAL: Master lcore 0 is ready (tid=57f7040;cpuset=[0])
    PMD: ENICPMD trace: rte_enic_pmd_init
    EAL: PCI device 0000:00:04.0 on NUMA socket -1
    EAL:   probe driver: 1af4:1000 rte_virtio_pmd
    APP: HPET is not enabled, using TSC as default timer
    RTE>>

Run another sample applications
--------------------------------------------

Delete osv-dpdk instance at first if you already deployed it on Capstan:

.. code-block:: console

    [user@host ~]$ cd DPDK-<version>/lib/librte_eal/osvapp/capstan/
    [user@host capstan]$ capstan delete osv-dpdk
    Deleted instance: osv-dpdk

Then you need to open Capstanfile on a editor, modify cmdline field:

.. code-block:: console

    base: cloudius/osv-base

    cmdline: --maxnic=0 /l2fwd --no-shconf -c 3 -n 2 --log-level 8 -m 768 -- -p 3

    build: ./GET

.. note::

	To control OSv instance via REST API, you'll need to specify '--maxnic=1'
	on cmdline, then attach one more NIC on virt-install.
	eth0 will exclusively use for REST server, DPDK uses other NICs.

Build VM image again:

.. code-block:: console

    [user@host capstan]$ capstan build osv-dpdk
    Building osv-dpdk...
    Downloading cloudius/osv-base/index.yaml...
    145 B / 145 B [======================================================] 100.00 %
    Downloading cloudius/osv-base/osv-base.qemu.gz...
    20.09 MB / 20.09 MB [================================================] 100.00 %
    Uploading files...
    10 / 10 [============================================================] 100.00 %

.. note::

	You can use another name for new VM instance.
	On that case, you don't have to delete existing instance.

Export VM image to libvirt
--------------------------------------------

Packet forwarding application(such as l2fwd or l3fwd) requires multiple vNICs with multiple bridges, but Capstan does not have a way to configure such network.

To do so, you can export VM image to libvirt by using virt-install:

.. code-block:: console

    [user@host ~]$ sudo virt-install --import --noreboot --name=osv-dpdk --ram=4096 --vcpus=2 --disk path=/home/user/.capstan/repository/osv-dpdk/osv-dpdk.qemu,bus=virtio --os-variant=none --accelerate --network=network:default,model=virtio --network=network:net2,model=virtio --serial pty --cpu host --rng=/dev/random

    WARNING  Graphics requested but DISPLAY is not set. Not running virt-viewer.
    WARNING  No console to launch for the guest, defaulting to --wait -1

    Starting install...
    Creating domain...                                          |    0 B  00:00
    Domain creation completed. You can restart your domain by running:
      virsh --connect qemu:///system start osv-dpdk

    [user@host ~]$ sudo virsh start osv-dpdk;sudo virsh console osv-dpdkDomain osv-dpdk started

    Connected to domain osv-dpdk
    Escape character is ^]
    OSv v0.19
    eth1: 192.168.123.63
    EAL: Detected lcore 0 as core 0 on socket 0
    EAL: Detected lcore 1 as core 1 on socket 0
    EAL: Support maximum 128 logical core(s) by configuration.
    EAL: Detected 2 lcore(s)
    EAL:    bar2 not available
    EAL:    bar2 not available
    EAL:    bar2 not available
    EAL:    bar1 not available
    EAL:    bar2 not available
    EAL:    bar1 not available
    EAL:    bar4 not available
    EAL:    bar0 not available
    EAL:    bar1 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL:    bar1 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL:    bar0 not available
    EAL: PCI scan found 16 devices
    EAL: Setting up memory...
    EAL: Mapped memory segment 0 @ 0xffff80013e000000: physaddr:0x13e000000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80013be00000: physaddr:0x13be00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800139c00000: physaddr:0x139c00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800137a00000: physaddr:0x137a00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800135800000: physaddr:0x135800000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800133600000: physaddr:0x133600000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800131400000: physaddr:0x131400000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80012f200000: physaddr:0x12f200000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80012d000000: physaddr:0x12d000000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80012ae00000: physaddr:0x12ae00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800128c00000: physaddr:0x128c00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800126a00000: physaddr:0x126a00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800124800000: physaddr:0x124800000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800122600000: physaddr:0x122600000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800120400000: physaddr:0x120400000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80011e200000: physaddr:0x11e200000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80011c000000: physaddr:0x11c000000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800119e00000: physaddr:0x119e00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800117c00000: physaddr:0x117c00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800115a00000: physaddr:0x115a00000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800113800000: physaddr:0x113800000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff800111600000: physaddr:0x111600000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff80010f400000: physaddr:0x10f400000, len 33554432
    EAL: Mapped memory segment 0 @ 0xffff8000bde00000: physaddr:0xbde00000, len 33554432
    EAL: TSC frequency is ~1575941 KHz
    EAL: Master lcore 0 is ready (tid=4b76040;cpuset=[0])
    PMD: ENICPMD trace: rte_enic_pmd_init
    EAL: lcore 1 is ready (tid=52fe040;cpuset=[1])
    EAL: PCI device 0000:00:03.0 on NUMA socket -1
    EAL:   probe driver: 1af4:1000 rte_virtio_pmd
    EAL: PCI device 0000:00:04.0 on NUMA socket -1
    EAL:   probe driver: 1af4:1000 rte_virtio_pmd
    Lcore 0: RX port 0
    Lcore 1: RX port 1
    Initializing port 0... done:
    Port 0, MAC address: 52:54:00:05:59:A9

    Initializing port 1... done:
    Port 1, MAC address: 52:54:00:38:65:DA


    Checking link statusdone
    Port 0 Link Up - speed 10000 Mbps - full-duplex
    Port 1 Link Up - speed 10000 Mbps - full-duplex
    L2FWD: entering main loop on lcore 1
    L2FWD: entering main loop on lcore 0
    L2FWD:  -- lcoreid=1 portid=1
    L2FWD:  -- lcoreid=0 portid=0

    Port statistics ====================================
    Statistics for port 0 ------------------------------
    Packets sent:                        0
    Packets received:                    0
    Packets dropped:                     0
    Statistics for port 1 ------------------------------
    Packets sent:                        0
    Packets received:                    0
    Packets dropped:                     0
    Aggregate statistics ===============================
    Total packets sent:                  0
    Total packets received:              0
    Total packets dropped:               0
    ====================================================

