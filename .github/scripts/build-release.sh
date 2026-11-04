#!/usr/bin/env bash
set -euo pipefail

source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
build_dir="$source_dir/build/release"
music_source="$source_dir/build/deps/zmusic"
vcpkg_dir="$source_dir/build/deps/vcpkg"
configuration=Release
common=(-DCMAKE_POLICY_VERSION_MINIMUM=3.5)
music_options=(-DBUILD_SHARED_LIBS=ON)
raze_options=(-DPK3_QUIET_ZIPDIR=ON -DDYN_OPENAL=OFF)

case "${RELEASE_PLATFORM:?}/${RELEASE_ARCHITECTURE:?}" in
  windows/x86_64)
    configuration=RelWithDebInfo
    # Keep optimization enabled and produce complete PDBs for these exact binaries.
    common+=(
      -G "Visual Studio 17 2022" -A x64
      "-DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG:FULL /INCREMENTAL:NO"
      "-DCMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO=/DEBUG:FULL /INCREMENTAL:NO"
    )
    music_triplet=x64-windows
    raze_triplet=x64-windows-static
    music_library="$build_dir/zmusic/source/$configuration/zmusiclite.lib"
    raze_options+=(-DLIBVPX_VCPKG=ON)
    ;;
  macos/x86_64|macos/arm64)
    common+=(
      -G Ninja "-DCMAKE_OSX_ARCHITECTURES=$RELEASE_ARCHITECTURE"
      "-DVCPKG_OVERLAY_TRIPLETS=$build_dir/triplets"
    )
    if [[ "$RELEASE_ARCHITECTURE" == arm64 ]]; then
      raze_triplet=arm64-osx
    else
      raze_triplet=x64-osx
    fi
    music_triplet="$raze_triplet-dynamic"
    music_library="$build_dir/zmusic/source/libzmusiclite.dylib"
    raze_options+=(-DMOLTENVK_VCPKG=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0)
    ;;
  linux/x86_64)
    common+=(-G Ninja)
    music_library="$build_dir/zmusic/source/libzmusiclite.so"
    # Link codecs explicitly so missing libraries are detected when packaging.
    music_options+=(-DDYN_SNDFILE=OFF -DDYN_MPG123=OFF)
    ;;
  *)
    echo "Unsupported release target: $RELEASE_PLATFORM/$RELEASE_ARCHITECTURE" >&2
    exit 1
    ;;
esac
common+=("-DCMAKE_BUILD_TYPE=$configuration")

if [[ "$RELEASE_PLATFORM" != linux ]]; then
  cmake -P "$source_dir/.github/scripts/release-dependencies.cmake"
  common+=(
    "-DCMAKE_TOOLCHAIN_FILE=$vcpkg_dir/scripts/buildsystems/vcpkg.cmake"
    "-DVCPKG_OVERLAY_PORTS=$source_dir/vcpkg-ports"
  )
  music_options+=(
    "-DVCPKG_TARGET_TRIPLET=$music_triplet" -DVCPKG_LIBSNDFILE=ON
    "-DVCPKG_MANIFEST_DIR=$build_dir/zmusic-manifest"
  )
  raze_options+=(
    "-DVCPKG_TARGET_TRIPLET=$raze_triplet" -DOPENAL_SOFT_VCPKG=ON
    "-DVCPKG_MANIFEST_DIR=$build_dir/raze-manifest"
  )
fi

cmake -S "$music_source" -B "$build_dir/zmusic" \
  "${common[@]}" "${music_options[@]}"
cmake --build "$build_dir/zmusic" --config "$configuration" --target zmusiclite --parallel 3

cmake -S "$source_dir" -B "$build_dir/raze" \
  "${common[@]}" "${raze_options[@]}" \
  "-DZMUSIC_INCLUDE_DIR=$music_source/include" \
  "-DZMUSIC_LIBRARIES=$music_library"
cmake --build "$build_dir/raze" --config "$configuration" --parallel 3
