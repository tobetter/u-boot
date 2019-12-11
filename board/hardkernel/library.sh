#!/bin/sh
#
# Copyright (C) 2019 Dongjin Kim <tobetter@gmail.com>
#

warning() {
	echo "W: $1"
}

panic() {
	echo "E: $1"
	exit 1
}

lookup_by_mount() {
	local mount=$(grep "\ $1\ " /proc/mounts | cut -d' ' -f1)
	echo ${mount%p*}
}

do_fusing_exynos5422() {
	local bindir=/usr/lib/u-boot/odroid-xu3

	[ -b $1 ] || panic "Invalid block device."

	if [ -d /sys/block/${1##*/}boot0 ]; then
		echo "$1 is an eMMC card, disabling ${1##*/}boot0 ro"
		echo -n 0 | tee /sys/block/${1##*/}boot0/force_ro || \
			warning "Enabling r/w for $1boot0 failed"
		bl1_position=0
		bl2_position=30
		uboot_position=62
		tzsw_position=2110
		device=$1boot0
	else
		bl1_position=1
		bl2_position=31
		uboot_position=63
		tzsw_position=2111
		device=$1
	fi

	dd if=$bindir/bl1.bin.hardkernel of=$device seek=$bl1_position conv=fsync
	dd if=$bindir/bl2.bin.hardkernel.1mb_uboot of=$device seek=$bl2_position conv=fsync
	dd if=$bindir/tzsw.bin.hardkernel of=$device seek=$tzsw_position conv=fsync
	dd if=$bindir/u-boot.bin of=$device seek=$uboot_position conv=fsync

	if [ -d /sys/block/${1##*/}boot0 ]; then
		echo -n 1 | tee /sys/block/${1##*/}boot0/force_ro || \
			warning "Enabling r/w for $1boot0 failed"
	fi

	echo "U-boot image is fused successfully."
}

do_fusing() {
	[ -b $1 ] || panic "Invalid block device."

	case $(cat /proc/device-tree/model) in
		"Hardkernel Odroid XU4")
			do_fusing_exynos5422 $1
			;;
		*)
			warning "Unknown board."
			;;
	esac
}
