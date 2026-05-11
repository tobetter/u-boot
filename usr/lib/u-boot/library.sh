abort() {
	echo $1
	exit 1
}

flash_binary()
{
	device=${1}
	target=${2}
	path=/usr/lib/u-boot/${target}

	echo -- ${path}/sd_fusing.sh
	[ -f ${path}/sd_fusing.sh ] || return

	case ${device} in /dev/mmcblk*)
		files=$(ls ${path} --ignore="*.md5sum" | sort -V)
		(cd /usr/lib/u-boot/${target};
			md5sum ${files} > files-new.md5sum;
			if ! diff files.md5sum files-new.md5sum 2>/dev/null; then
				sh ./sd_fusing.sh ${device};
				cp files-new.md5sum files.md5sum;
			fi;
			rm -f files-new.md5sum;)
		;;
	*)
		echo "W: U-boot can be flashed to MMC/SD memory card only..."
		;;
	esac
}

lookup_by_mount() {
	local mount=$(grep "\ $1\ " /proc/mounts | cut -d' ' -f1)
	echo ${mount%p*}
}

find_boot_device()
{
	if [ "x$BOOT_DEVICE" != "x" ]; then
		echo ${BOOT_DEVICE}
		return
	fi

	dev=$(lookup_by_mount "/target/boot")
	[ "x${dev}" = "x" ] && dev=$(lookup_by_mount "/boot")

	dev=$(readlink -f ${dev})
	echo ${dev%p*}
}

find_root_device()
{
	find_boot_device
}
