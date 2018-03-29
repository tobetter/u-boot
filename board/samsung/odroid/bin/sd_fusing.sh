#!/bin/sh
#
# Copyright (C) Dongjin Kim <tobetter@gmail.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

set -x

. $(dirname "$0")/functions

dev="$1"

[ -z ${binary_dir} ] && binary_dir="$(dirname $0)"
bl1=${binary_dir}/bl1.bin.hardkernel
bl2=${binary_dir}/bl2.bin.hardkernel
uboot=${binary_dir}/u-boot.bin

flash_bootloader ${dev} ${bl1} ${bl2} ${uboot}
eject $1
echo Finished.
