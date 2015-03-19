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

.. _compiling_sample_apps:

Compiling and Running Sample Applications
=========================================

The chapter describes how to compile and run applications in a DPDK
environment. It also provides a pointer to where sample applications are stored.

Running a Sample Application
----------------------------

The following is the list of options that can be given to the EAL:

.. code-block:: console

    ./rte-app -c COREMASK -n NUM [-b <domain:bus:devid.func>] [-r NUM] [-v] [--proc-type <primary|secondary|auto>]

.. note::

    EAL has a common interface between all operating systems and is based on the
    Linux* notation for PCI devices. For example, a FreeBSD* device selector of
    pci0:2:0:1 is referred to as 02:00.1 in EAL.

The EAL options for FreeBSD* are as follows:

*   -c COREMASK
    : A hexadecimal bit mask of the cores to run on.  Note that core numbering
    can change between platforms and should be determined beforehand.

*   -n NUM
    : Number of memory channels per processor socket.

*   -b <domain:bus:devid.func>
    : blacklisting of ports; prevent EAL from using specified PCI device
    (multiple -b options are allowed).

*   --use-device
    : use the specified ethernet device(s) only.  Use comma-separate
    <[domain:]bus:devid.func> values. Cannot be used with -b option.

*   -r NUM
    : Number of memory ranks.

*   -v
    : Display version information on startup.

*   --proc-type
    : The type of process instance.

Other options, specific to Linux* and are not supported under FreeBSD* are as follows:

*   socket-mem
    : Memory to allocate from hugepages on specific sockets.

*   --huge-dir
    : The directory where hugetlbfs is mounted.

*   --file-prefix
    : The prefix text used for hugepage filenames.

*   -m MB
    : Memory to allocate from hugepages, regardless of processor socket.
    It is recommended that --socket-mem be used instead of this option.

The -c and the -n options are mandatory; the others are optional.

Edit cmdline on Capstanfile, then rebuild and run VM instance as follows
(assuming the platform has four memory channels, and that cores 0-3
are present and are to be used for running the application):

.. code-block:: console

    [user@host ~]$ cd DPDK-<version>/lib/librte_eal/osvapp/capstan/
    [user@host ~]$ vi Capstanfile # edit cmdline
    [user@host capstan]$ capstan delete osv-dpdk
    Deleted instance: osv-dpdk
    [user@host capstan]$ capstan build osv-dpdk
    Building osv-dpdk...
    Downloading cloudius/osv-base/index.yaml...
    145 B / 145 B [======================================================] 100.00 %
    Downloading cloudius/osv-base/osv-base.qemu.gz...
    20.09 MB / 20.09 MB [================================================] 100.00 %
    Uploading files...
    10 / 10 [============================================================] 100.00 %
    [user@host ~]$ capstan run osv-dpdk

.. note::

    The --proc-type and --file-prefix EAL options are used for running multiple
    DPDK processes.  See the “Multi-process Sample Application” chapter
    in the *DPDK Sample Applications User Guide and the DPDK
    Programmers Guide* for more details.

