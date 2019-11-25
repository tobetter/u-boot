DOWNLOAD_URL="https://api.github.com/repos/hardkernel/u-boot/releases/latest"
TARBALL_FILE="/tmp/uboot.tar.gz"
UBOOT_BINARY="/usr/lib/u-boot/odroid-n2/u-boot.bin"

abort() {
	echo $1
	exit 1
}

download_binary()
{
	mkdir -p ${binary_dir}
	curl -sL ${DOWNLOAD_URL} | grep browser_download_url | \
		cut -d'"' -f4 | wget -qi - -O ${TARBALL_FILE} || abort "Failed to download"

	tar xzf ${TARBALL_FILE} -C ${binary_dir} --strip-components=1
	[ -f ${UBOOT_BINARY} ] || abort "E: U-boot binary is missing"
}

flash_binary()
{
	dev=${1}
	binary=${2}

	dd if=${binary} of=${dev} conv=fsync,notrunc bs=512 seek=1
}

find_root_device()
{
	for p in $(cat /proc/cmdline); do
		case $p in
			root=UUID=*)
				uuid=${p#root=UUID=}
				if [ "x${uuid}" != "x" ]; then
					dev=$(blkid | grep ${uuid} | cut -d':' -f1)
					echo ${dev%p*}
					return
				fi
				;;
			*)
				;;
		esac
	done
}
