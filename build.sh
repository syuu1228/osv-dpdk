#!/bin/sh

rm -rf ROOTFS usr.manifest x86_64-default-osvapp-gcc examples/*/build/*
make install T=x86_64-default-osvapp-gcc OSV_SDK=`readlink -f ../..` || exit $?
(cd examples/cmdline; env RTE_SDK=`readlink -f ../../` RTE_TARGET=x86_64-default-osvapp-gcc make)
(cd examples/helloworld; env RTE_SDK=`readlink -f ../../` RTE_TARGET=x86_64-default-osvapp-gcc make)
(cd examples/ip_fragmentation; env RTE_SDK=`readlink -f ../../` RTE_TARGET=x86_64-default-osvapp-gcc make)
(cd examples/l2fwd; env RTE_SDK=`readlink -f ../../` RTE_TARGET=x86_64-default-osvapp-gcc make)
(cd examples/l3fwd; env RTE_SDK=`readlink -f ../../` RTE_TARGET=x86_64-default-osvapp-gcc make)
mkdir -p ROOTFS
find x86_64-default-osvapp-gcc/app/ -executable -readable -type f -exec cp -v {} ROOTFS/ \;
find examples/*/build/app -executable -readable -type f -exec cp -v {} ROOTFS/ \;
for i in ROOTFS/*
do echo /${i#ROOTFS/}: \${MODULE_DIR}/$i
done > usr.manifest
