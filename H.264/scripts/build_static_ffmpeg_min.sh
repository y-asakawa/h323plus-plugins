#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-1.0
set -euo pipefail

FFMPEG_VERSION="${FFMPEG_VERSION:-7.1.1}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
if [[ -z "${H323PLUS_ROOT:-}" ]]; then
  if [[ -f "${PLUGIN_ROOT}/../../../include/codec/opalplugin.h" ]]; then
    H323PLUS_ROOT="$(cd "${PLUGIN_ROOT}/../../.." && pwd)"
  else
    H323PLUS_ROOT="$(cd "${PLUGIN_ROOT}/../../h323plus" && pwd)"
  fi
fi
OUT_ROOT="${1:-${H323PLUS_ROOT}/third_party/static_ffmpeg}"
SRC_ROOT="${OUT_ROOT}/src"
INSTALL_ROOT="${OUT_ROOT}/install"
ARCHIVE="ffmpeg-${FFMPEG_VERSION}.tar.xz"
SRC_DIR="${SRC_ROOT}/ffmpeg-${FFMPEG_VERSION}"
URL="https://ffmpeg.org/releases/${ARCHIVE}"

mkdir -p "${SRC_ROOT}"
cd "${SRC_ROOT}"

if [[ ! -f "${ARCHIVE}" ]]; then
  curl -L -o "${ARCHIVE}" "${URL}"
fi

if [[ ! -d "${SRC_DIR}" ]]; then
  tar xf "${ARCHIVE}"
fi

cd "${SRC_DIR}"

./configure \
  --prefix="${INSTALL_ROOT}" \
  --enable-static \
  --disable-shared \
  --enable-pic \
  --disable-programs \
  --disable-doc \
  --disable-network \
  --disable-autodetect \
  --disable-avdevice \
  --disable-avfilter \
  --disable-postproc \
  --disable-swresample \
  --disable-swscale \
  --disable-avformat \
  --disable-everything \
  --enable-avcodec \
  --enable-avutil \
  --enable-decoder=h264 \
  --enable-decoder=hevc \
  --enable-parser=h264

make -j"$(sysctl -n hw.ncpu)"
make install

echo "Built static ffmpeg into: ${INSTALL_ROOT}"
