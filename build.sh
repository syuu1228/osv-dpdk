#!/bin/sh

rm -rf ROOTFS usr.manifest x86_64-default-osvapp-gcc
make install T=x86_64-default-osvapp-gcc OSV_SDK=`readlink -f ../..` || exit $?
mkdir -p ROOTFS
cp -a x86_64-default-osvapp-gcc/app/* ROOTFS/
for i in ROOTFS/*
do echo /${i#ROOTFS/}: \${MODULE_DIR}/$i
done > usr.manifest
