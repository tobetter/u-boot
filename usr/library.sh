TMPDIR="/tmp/u-boot"

DOWNLOAD_URL="https://api.github.com/repos/hardkernel/u-boot/releases/latest"
TARBALL_FILE="${TMPDIR}/odroid-n2.tar.gz"
UBOOT_BINARY="${TMPDIR}/sd_fuse/u-boot.bin"

abort() {
	rm -f ${TMPDIR}
	echo $1
	exit 1
}

download_binary()
{
	mkdir -p ${TMPDIR}
	curl -sL ${DOWNLOAD_URL} | \
		jq -r '.assets[].browser_download_url' | \
		wget -qi - -O ${TARBALL_FILE} || abort "Failed to download"

	tar xzf ${TARBALL_FILE} -C ${TMPDIR}
	[ -f ${UBOOT_BINARY} ] || abort XXXX
}

flash_binary()
{
	dev=${1}
	binary=${2}

	echo dd if=${binary} of=${dev} conv=fsync,notrunc bs=512 seek=1
}
