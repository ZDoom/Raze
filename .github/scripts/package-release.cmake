cmake_minimum_required(VERSION 3.21)

if(NOT RELEASE_VERSION MATCHES "^(v[0-9]+\\.[0-9]+\\.[0-9]+(-test\\.[0-9]+)?|ci-[0-9]+-[0-9]+)$")
    message(FATAL_ERROR "Invalid RELEASE_VERSION: ${RELEASE_VERSION}")
endif()
if(NOT "${RELEASE_PLATFORM}/${RELEASE_ARCHITECTURE}" MATCHES
        "^(windows/x86_64|linux/x86_64|macos/x86_64|macos/arm64)$")
    message(FATAL_ERROR "Unsupported release target")
endif()

get_filename_component(source_dir "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(build_dir "${source_dir}/build/release")
set(music_source "${source_dir}/build/deps/zmusic")
set(package_name "raze-${RELEASE_VERSION}-${RELEASE_PLATFORM}-${RELEASE_ARCHITECTURE}")
set(stage "${build_dir}/stage/${package_name}")
file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}" "${build_dir}/dist")

function(require_file filename)
    if(NOT EXISTS "${filename}")
        message(FATAL_ERROR "Missing release file: ${filename}")
    endif()
    file(SIZE "${filename}" size)
    if(size EQUAL 0)
        message(FATAL_ERROR "Empty release file: ${filename}")
    endif()
endfunction()

function(run_checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Command failed (${result}): ${ARGV}")
    endif()
endfunction()

file(COPY "${source_dir}/package/common/" DESTINATION "${stage}")
file(COPY "${source_dir}/README.md" "${source_dir}/package/release/README.txt"
    DESTINATION "${stage}")
file(COPY "${music_source}/licenses/" DESTINATION "${stage}/licenses/ZMusic")
# Keep notices from both manifests: Raze's static libraries and ZMusic's codecs.
foreach(project raze zmusic)
    file(GLOB notices "${build_dir}/${project}/vcpkg_installed/*/share/*/copyright")
    foreach(notice IN LISTS notices)
        get_filename_component(port_dir "${notice}" DIRECTORY)
        get_filename_component(port "${port_dir}" NAME)
        file(COPY "${notice}" DESTINATION "${stage}/licenses/${project}/${port}")
    endforeach()
endforeach()

if(RELEASE_PLATFORM STREQUAL "macos")
    set(app "${stage}/raze.app")
    file(COPY "${build_dir}/raze/raze.app" DESTINATION "${stage}")
    set(runtime "${app}/Contents/MacOS")
    require_file("${runtime}/raze")
    require_file("${runtime}/libMoltenVK.dylib")
    require_file("${app}/Contents/Resources/licenses/MoltenVK/copyright")
    # codesign treats subdirectories in MacOS as nested code. Keep data in
    # Resources, with the relative path that Raze's soundfont search expects.
    file(RENAME "${runtime}/soundfonts" "${app}/Contents/Resources/soundfonts")
    file(CREATE_LINK "../Resources/soundfonts" "${runtime}/soundfonts" SYMBOLIC)
    file(RENAME "${runtime}/raze.pk3" "${app}/Contents/Resources/raze.pk3")
    file(COPY "${source_dir}/package/common/gamecontrollerdb.txt" DESTINATION "${app}/Contents/Resources")
    foreach(resource raze.pk3 gamecontrollerdb.txt)
        file(CREATE_LINK "../Resources/${resource}" "${runtime}/${resource}" SYMBOLIC)
    endforeach()

    # Resolve ZMusic and its codec libraries, replacing build-machine paths.
    # MoltenVK is loaded by dlopen and already has a valid upstream signature.
    include(BundleUtilities)
    file(GLOB codec_dirs "${build_dir}/zmusic/vcpkg_installed/*/lib")
    fixup_bundle("${app}" "" "${build_dir}/zmusic/source;${codec_dirs}"
        IGNORE_ITEM libMoltenVK.dylib)
    require_file("${app}/Contents/Frameworks/libzmusiclite.1.dylib")

    set(bundle_version "0.0.0")
    if(RELEASE_VERSION MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(bundle_version "${CMAKE_MATCH_1}")
    endif()
    run_checked(/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString ${bundle_version}"
        "${app}/Contents/Info.plist")
    run_checked(/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion 15.0"
        "${app}/Contents/Info.plist")
    # Re-sign modified Mach-O files, then seal and verify the app bundle.
    file(GLOB_RECURSE dylibs "${app}/*.dylib")
    foreach(dylib IN LISTS dylibs)
        if(NOT dylib MATCHES "/libMoltenVK\\.dylib$")
            run_checked(codesign --force --sign - "${dylib}")
        endif()
    endforeach()
    run_checked(codesign --force --sign - "${app}")
    run_checked(codesign --verify --deep --strict "${app}")
    run_checked(lipo "${runtime}/raze" -verify_arch "${RELEASE_ARCHITECTURE}")
elseif(RELEASE_PLATFORM STREQUAL "windows")
    set(runtime "${stage}")
    set(configuration RelWithDebInfo)
    file(COPY "${build_dir}/raze/${configuration}/raze.exe" DESTINATION "${stage}")
    file(COPY "${build_dir}/zmusic/source/${configuration}/zmusiclite.dll" DESTINATION "${stage}")
    file(GLOB dlls "${build_dir}/zmusic/vcpkg_installed/x64-windows/bin/*.dll")
    if(NOT dlls)
        message(FATAL_ERROR "ZMusic codec DLLs were not found")
    endif()
    file(COPY ${dlls} DESTINATION "${stage}")
    require_file("${stage}/raze.exe")
    require_file("${stage}/zmusiclite.dll")
    # The dynamically linked codec DLLs use the Visual C++ runtime.
    file(GLOB crt_dirs "$ENV{VCToolsRedistDir}/x64/Microsoft.VC*.CRT")
    if(NOT crt_dirs)
        execute_process(COMMAND "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer/vswhere.exe"
            -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
            -property installationPath OUTPUT_VARIABLE vs_dir OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        file(GLOB crt_dirs "${vs_dir}/VC/Redist/MSVC/*/x64/Microsoft.VC*.CRT")
    endif()
    if(NOT crt_dirs)
        message(FATAL_ERROR "Visual C++ redistributable DLLs were not found")
    endif()
    list(SORT crt_dirs COMPARE NATURAL ORDER DESCENDING)
    list(GET crt_dirs 0 crt_dir)
    file(GLOB crt_dlls "${crt_dir}/*.dll")
    file(COPY ${crt_dlls} DESTINATION "${stage}")
    require_file("${stage}/vcruntime140.dll")
    find_program(python NAMES python python3 REQUIRED)
    execute_process(COMMAND "${python}" -c [=[
import ctypes
import os
import pathlib
import struct
import sys

folder = pathlib.Path(sys.argv[1])
binary = (folder / "raze.exe").read_bytes()
pe = struct.unpack_from("<I", binary, 0x3c)[0]
assert binary[pe:pe + 4] == b"PE\0\0"
assert struct.unpack_from("<H", binary, pe + 4)[0] == 0x8664, "Expected an x64 executable"
with os.add_dll_directory(str(folder)):
    ctypes.WinDLL(str(folder / "zmusiclite.dll"))
]=] "${stage}" COMMAND_ERROR_IS_FATAL ANY)

    # Package symbols from the same build as the shipped executable and DLL.
    set(pdb_name "${package_name}-pdb")
    set(pdb_stage "${build_dir}/stage/${pdb_name}")
    file(REMOVE_RECURSE "${pdb_stage}")
    file(MAKE_DIRECTORY "${pdb_stage}")
    set(raze_pdb "${build_dir}/raze/${configuration}/raze.pdb")
    set(music_pdb "${build_dir}/zmusic/source/${configuration}/zmusiclite.pdb")
    require_file("${raze_pdb}")
    require_file("${music_pdb}")
    file(COPY "${raze_pdb}" "${music_pdb}" DESTINATION "${pdb_stage}")
    file(GLOB codec_pdbs "${build_dir}/zmusic/vcpkg_installed/x64-windows/bin/*.pdb")
    if(codec_pdbs)
        file(COPY ${codec_pdbs} DESTINATION "${pdb_stage}")
    endif()
    file(WRITE "${pdb_stage}/README.txt"
        "Debug symbols for ${package_name}.zip\n"
        "These PDBs match the binaries in that archive. Extract them beside the\n"
        "executable and DLLs, or add this directory to your debugger's symbol path.\n")
    set(pdb_archive "${build_dir}/dist/${pdb_name}.7z")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${pdb_archive}" --format=7zip "${pdb_name}"
        WORKING_DIRECTORY "${build_dir}/stage" COMMAND_ERROR_IS_FATAL ANY)
    require_file("${pdb_archive}")
    message(STATUS "Debug symbols archive: ${pdb_archive}")
else()
    set(runtime "${stage}")
    file(COPY "${build_dir}/raze/raze" DESTINATION "${stage}")
    file(GLOB music_libs "${build_dir}/zmusic/source/libzmusiclite.so*")
    file(COPY ${music_libs} DESTINATION "${stage}")
    require_file("${stage}/raze")
    require_file("${stage}/libzmusiclite.so.1")
    execute_process(COMMAND ldd "${stage}/raze" RESULT_VARIABLE result
        OUTPUT_VARIABLE dependencies ERROR_VARIABLE errors)
    if(NOT result EQUAL 0 OR dependencies MATCHES "not found")
        message(FATAL_ERROR "Unresolved Linux dependencies:\n${dependencies}\n${errors}")
    endif()
    message(STATUS "Linux runtime dependencies:\n${dependencies}")
endif()

if(NOT RELEASE_PLATFORM STREQUAL "macos")
    file(COPY "${build_dir}/raze/raze.pk3" DESTINATION "${runtime}")
    file(COPY "${source_dir}/soundfont/raze.sf2" DESTINATION "${runtime}/soundfonts")
endif()
require_file("${runtime}/raze.pk3")
require_file("${runtime}/soundfonts/raze.sf2")

if(RELEASE_PLATFORM STREQUAL "macos")
    set(archive "${build_dir}/dist/${package_name}.zip")
    run_checked(ditto -c -k --sequesterRsrc --keepParent "${stage}" "${archive}")
elseif(RELEASE_PLATFORM STREQUAL "windows")
    set(archive "${build_dir}/dist/${package_name}.zip")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${archive}" --format=zip "${package_name}"
        WORKING_DIRECTORY "${build_dir}/stage" COMMAND_ERROR_IS_FATAL ANY)
else()
    set(archive "${build_dir}/dist/${package_name}.tar.gz")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${archive}" "${package_name}"
        WORKING_DIRECTORY "${build_dir}/stage" COMMAND_ERROR_IS_FATAL ANY)
endif()
require_file("${archive}")
message(STATUS "Release archive: ${archive}")
