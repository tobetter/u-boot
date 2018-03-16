#!/bin/sh
#
# Copyright (C) 2015-2018 Hardkernel Co,. Ltd
# Dongjin Kim <tobetter@gmail.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

set -e

. $(dirname "$0")/functions

dev="$1"

[ -z ${binary_dir} ] && binary_dir="$(dirname $0)"
bl1=${binary_dir}/bl1.bin.hardkernel
uboot=${binary_dir}/u-boot.bin.signed

flash_bootloader ${bl1} ${uboot} ${dev}
eject $1
echo Finished.
