#!/bin/sh

# Make a new device node
/bin/rm /dev/fpga_int > /dev/null
/bin/mknod /dev/fpga_int c 200 0

#
# Load the kernel module:
#
/sbin/rmmod fpga_int
/sbin/insmod ./fpga_int.ko

if [ $? != 0 ]; then
    printf "$0: *** Error: insmod failed\n"
    exit 1
fi

