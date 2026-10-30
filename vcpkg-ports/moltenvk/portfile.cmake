# Raze loads MoltenVK with dlopen(), including when its other dependencies are static.
vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

# The binary already uses @rpath and only depends on system frameworks.
# Rewriting its load commands would invalidate the upstream code signature.
set(VCPKG_FIXUP_MACHO_RPATH OFF)

# The official universal binary avoids requiring a full Xcode installation.
vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/KhronosGroup/MoltenVK/releases/download/v${VERSION}/MoltenVK-macos.tar"
    FILENAME "MoltenVK-macos-${VERSION}.tar"
    SHA512 985f483e832ae3605b62b78798989f79c8833c13b568ff3615fb9c1f2780c3e9960cb13f79f2560b01563694249520d23d4f93d284580c13f6c842eff0c6b99d
)
vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

set(MOLTENVK_LIBRARY "${SOURCE_PATH}/MoltenVK/dynamic/dylib/macOS/libMoltenVK.dylib")
file(INSTALL "${MOLTENVK_LIBRARY}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
if(NOT VCPKG_BUILD_TYPE)
    file(INSTALL "${MOLTENVK_LIBRARY}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
endif()
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

# This package supplies only the runtime; Raze already includes its Vulkan headers.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
