#!/bin/bash
set -euo pipefail

cd -- "$(dirname -- "$0")"

readonly board=ipq9574_xiaomi_be7000
readonly defconfig="$board"_defconfig
readonly output="bin/openwrt-$board-u-boot.mbn"

usage() {
	printf 'Usage: %s [%s|%s|clean|clean_all]\n' "$0" "$board" "$defconfig"
}

export ARCH=arm
export CROSS_COMPILE=arm-openwrt-linux-
export STAGING_DIR="$(pwd -P)/../staging_dir/"
export PATH="${STAGING_DIR}toolchain-arm_cortex-a7_gcc-5.2.0_musl-1.1.16_eabi/bin:$PATH"
export HOSTLDFLAGS="-L${STAGING_DIR}usr/lib -znow -zrelro -pie"
export TARGETCC="${CROSS_COMPILE}gcc"

if (( $# > 1 )); then
	usage >&2
	exit 2
fi

case "${1:-$board}" in
	-h|--help)
		usage
		exit 0
		;;
	clean_all)
		rm -f -- "$output"
		exit 0
		;;
	clean)
		make mrproper
		rm -f -- "$output" combined_dtb.lds dtb_combined dtb_combined.bin.gz \
			dtb_combined.map arch/arm/dts/dtbtable.S \
			arch/arm/dts/compressed_dtb/dtbcombined.S failsafe/fsdata.c \
			tools/mbn_tools.pyc
		rm -rf -- failsafe/html/debug_output
		exit 0
		;;
	"$board"|"$defconfig")
		;;
	*)
		usage >&2
		exit 2
		;;
esac

if command -v python3 >/dev/null 2>&1; then
	python=python3
	converter=tools/elftombn_py3.py
elif command -v python2.7 >/dev/null 2>&1; then
	python=python2.7
	converter=tools/elftombn.py
else
	printf 'Python 3 or Python 2.7 is required to create the MBN image.\n' >&2
	exit 1
fi

for tool in "$TARGETCC" "${CROSS_COMPILE}strip"; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		printf 'Required tool is missing: %s. Install the toolchain in ../staging_dir.\n' "$tool" >&2
		exit 1
	fi
done

make clean
make "$defconfig"
make -j"$(nproc)"

mkdir -p -- bin
package_dir=$(mktemp -d bin/.be7000.XXXXXX)
trap 'rm -rf -- "$package_dir"' EXIT

"${CROSS_COMPILE}strip" u-boot -o "$package_dir/u-boot.strip"
"$python" "$converter" -f "$package_dir/u-boot.strip" \
	-o "$package_dir/u-boot.mbn" -v 6 -a ipq9574
test -s "$package_dir/u-boot.mbn"
mv -f -- "$package_dir/u-boot.mbn" "$output"
printf 'Built %s\n' "$output"
