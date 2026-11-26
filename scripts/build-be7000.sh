#!/bin/bash
set -euo pipefail

release_version=${1-}
if [[ $# -gt 1 || ( $# -eq 1 && ! $release_version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ) ]]; then
	printf 'Usage: %s [major.minor.patch]\n' "$0" >&2
	exit 2
fi

cd -- "$(dirname -- "$0")/.."

readonly board=ipq9574_xiaomi_be7000
readonly defconfig="$board"_defconfig
output="bin/openwrt-$board-u-boot.mbn"
make_version=()
if [[ -n $release_version ]]; then
	output="bin/u-boot-xiaomi-be7000-$release_version.mbn"
	make_version=("UBOOTVERSION=$release_version")
fi
readonly output
readonly toolchain_commit=0daf0879219130c46812cd5029fbcc2940316b16

export ARCH=arm
export CROSS_COMPILE=arm-openwrt-linux-
export STAGING_DIR="$(pwd -P)/../staging_dir/"
export PATH="${STAGING_DIR}toolchain-arm_cortex-a7_gcc-5.2.0_musl-1.1.16_eabi/bin:$PATH"
export HOSTLDFLAGS="-L${STAGING_DIR}usr/lib -znow -zrelro -pie"
export TARGETCC="${CROSS_COMPILE}gcc"

if [[ $(git -C "$STAGING_DIR" rev-parse HEAD 2>/dev/null) != "$toolchain_commit" ]]; then
	printf 'The toolchain in ../staging_dir must be at %s.\n' "$toolchain_commit" >&2
	exit 1
fi

source_commit=$(git rev-parse --verify HEAD)
export SOURCE_DATE_EPOCH="$(git show -s --format=%ct "$source_commit")"
export LC_ALL=C TZ=UTC
export LOCALVERSION="-g$(git rev-parse --short=12 "$source_commit")"
export NODE_PATH="$(pwd -P)/failsafe/html/node_modules"
node -e 'for (const name of Object.keys(require("./failsafe/html/package.json").dependencies)) require(name)'
command -v zopfli >/dev/null

command -v python3 >/dev/null

for tool in "$TARGETCC" "${CROSS_COMPILE}strip"; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		printf 'Required tool is missing: %s. Install the toolchain in ../staging_dir.\n' "$tool" >&2
		exit 1
	fi
done

make clean
make "$defconfig"
make -j"$(nproc)" "${make_version[@]}"

mkdir -p -- bin
package_dir=$(mktemp -d bin/.be7000.XXXXXX)
trap 'rm -rf -- "$package_dir"' EXIT

"${CROSS_COMPILE}strip" u-boot -o "$package_dir/u-boot.strip"
python3 tools/elftombn_py3.py -f "$package_dir/u-boot.strip" \
	-o "$package_dir/u-boot.mbn" -v 6 -a ipq9574
test -s "$package_dir/u-boot.mbn"
if [[ -n $release_version ]]; then
	embedded_version="${release_version}${LOCALVERSION}"
	if ! grep -aFq -- "U-Boot $embedded_version (" "$package_dir/u-boot.mbn"; then
		printf 'MBN does not contain the expected U-Boot version %s.\n' "$embedded_version" >&2
		exit 1
	fi
	printf 'Verified MBN version %s\n' "$embedded_version"
fi
mv -f -- "$package_dir/u-boot.mbn" "$output"
printf 'Built %s\n' "$output"
