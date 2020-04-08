#!/bin/sh

boards=$(cat debian/control | grep Package: | cut -d'-' -f3 | uniq | grep "^odroid.")

download_url="https://api.github.com/repos/hardkernel/u-boot/releases"
HK_TOKEN="odroidfun@gmail.com:c34f7703219d221954b2b79e47470a1897f52caa"

[ "x${HK_TOKEN}" = "x" ] && TOKEN="-u $(HK_TOKEN)"

get_url() {
	echo $(curl -sL ${download_url} ${TOKEN} | tr -d '"' \
		| grep browser_download_url \
		| sed -n -e 's/^.*browser_download_url: //p' \
		| grep ${1} \
		| sort -rV \
		| head -1)
}

for board in ${boards}; do
	url=$(get_url ${board})
	if [ ! "x${url}" = "x" ]; then
		wget ${url} -O uboot-${board}.tar.gz
		mkdir -p usr/lib/u-boot/${board}
		tar xzf uboot-${board}.tar.gz \
			-C usr/lib/u-boot/${board} --strip-components=1
		[ -f usr/lib/u-boot/${board}/sd_fusing.sh ] &&
			sed -i "/eject/d" usr/lib/u-boot/${board}/sd_fusing.sh
		rm -f uboot-${board}.tar.gz

		git add usr/lib/u-boot/${board}
		git add debian/u-boot-${board}.*

		debian/bin/build_targets ${board}
	fi
done

