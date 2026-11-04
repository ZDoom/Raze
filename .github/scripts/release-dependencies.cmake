cmake_minimum_required(VERSION 3.21)

get_filename_component(source_dir "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
execute_process(COMMAND git -C "${source_dir}/build/deps/vcpkg" rev-parse HEAD
    OUTPUT_VARIABLE baseline OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)

# Use the port versions from the workflow's pinned vcpkg checkout for both
# projects. Their original 2023 baselines predate current Apple SDKs. Generate
# build-local manifests so ordinary developer builds keep their own baselines.
foreach(project raze zmusic)
    if(project STREQUAL "raze")
        set(manifest_source "${source_dir}/vcpkg.json")
    else()
        set(manifest_source "${source_dir}/build/deps/zmusic/vcpkg.json")
    endif()
    file(READ "${manifest_source}" manifest)
    string(JSON manifest SET "${manifest}" builtin-baseline "\"${baseline}\"")
    set(destination "${source_dir}/build/release/${project}-manifest")
    file(MAKE_DIRECTORY "${destination}")
    file(WRITE "${destination}/vcpkg.json" "${manifest}\n")
endforeach()

if(CMAKE_HOST_APPLE)
    # Keep the minimum OS explicit even when a runner gains a newer SDK.
    foreach(arch arm64 x64)
        if(arch STREQUAL "x64")
            set(osx_arch x86_64)
        else()
            set(osx_arch arm64)
        endif()
        foreach(linkage static dynamic)
            set(triplet "${arch}-osx")
            if(linkage STREQUAL "dynamic")
                string(APPEND triplet "-dynamic")
            endif()
            file(WRITE "${source_dir}/build/release/triplets/${triplet}.cmake"
                "set(VCPKG_TARGET_ARCHITECTURE ${arch})\n"
                "set(VCPKG_CRT_LINKAGE dynamic)\n"
                "set(VCPKG_LIBRARY_LINKAGE ${linkage})\n"
                "set(VCPKG_CMAKE_SYSTEM_NAME Darwin)\n"
                "set(VCPKG_OSX_ARCHITECTURES ${osx_arch})\n"
                "set(VCPKG_OSX_DEPLOYMENT_TARGET 15.0)\n"
                "set(VCPKG_BUILD_TYPE release)\n")
        endforeach()
    endforeach()
endif()
