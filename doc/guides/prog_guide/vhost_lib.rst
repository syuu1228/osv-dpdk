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

Vhost Library
=============

The vhost cuse (cuse: user space character device driver) library implements a
vhost cuse driver. It also creates, manages and destroys vhost devices for
corresponding virtio devices in the guest. Vhost supported vSwitch could register
callbacks to this library, which will be called when a vhost device is activated
or deactivated by guest virtual machine.

Vhost API Overview
------------------

*   Vhost driver registration

      rte_vhost_driver_register registers the vhost cuse driver into the system.
      Character device file will be created in the /dev directory.
      Character device name is specified as the parameter.

*   Vhost session start

      rte_vhost_driver_session_start starts the vhost session loop.
      Vhost cuse session is an infinite blocking loop.
      Put the session in a dedicate DPDK thread.

*   Callback register

      Vhost supported vSwitch could call rte_vhost_driver_callback_register to
      register two callbacks, new_destory and destroy_device.
      When virtio device is activated or deactivated by guest virtual machine,
      the callback will be called, then vSwitch could put the device onto data
      core or remove the device from data core.

*   Read/write packets from/to guest virtual machine

      rte_vhost_enqueue_burst transmit host packets to guest.
      rte_vhost_dequeue_burst receives packets from guest.

*   Feature enable/disable

      Now one negotiate-able feature in vhost is merge-able.
      vSwitch could enable/disable this feature for performance consideration.

Vhost Implementation
--------------------

When vSwitch registers the vhost driver, it will register a cuse device driver
into the system and creates a character device file. This cuse driver will
receive vhost open/release/IOCTL message from QEMU simulator.

When the open call is received, vhost driver will create a vhost device for the
virtio device in the guest.

When VHOST_SET_MEM_TABLE IOCTL is received, vhost searches the memory region
to find the starting user space virtual address that maps the memory of guest
virtual machine. Through this virtual address and the QEMU pid, vhost could
find the file QEMU uses to map the guest memory. Vhost maps this file into its
address space, in this way vhost could fully access the guest physical memory,
which means vhost could access the shared virtio ring and the guest physical
address specified in the entry of the ring.

The guest virtual machine tells the vhost whether the virtio device is ready
for processing or is de-activated through VHOST_SET_BACKEND message.
The registered callback from vSwitch will be called.

When the release call is released, vhost will destroy the device.

Vhost supported vSwitch reference
---------------------------------

For how to support vhost in vSwitch, please refer to vhost example in the
DPDK Sample Applications Guide.
