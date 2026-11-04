Raze release builds
===================

Extract the entire archive before running Raze. Keep the executable, engine
resources, soundfonts and libraries together. Original game data is required;
commercial game files are not included in these downloads.

Windows (x86_64)
---------------
Run raze.exe. ZMusic, its codecs and the Visual C++ runtime are included.
OpenAL Soft is linked into the executable. Raze currently requires a 64-bit OS.
For crash debugging, download the matching windows-x86_64-pdb.7z archive from
the same release. Extract its PDBs beside the executable and DLLs, or add their
directory to your debugger's symbol search path.

macOS (Intel or Apple Silicon, macOS 15 or newer)
----------------------------------------------
Move raze.app to Applications and open it. The app includes ZMusic, its codecs,
OpenAL Soft and MoltenVK. These builds are ad-hoc signed, not notarized with an
Apple Developer ID. macOS may require approval in Privacy & Security on first
launch. Choose the archive for your Mac's processor.

Linux (x86_64, Ubuntu 22.04 runtime)
---------------------------------
Run ./raze from the extracted directory. ZMusic is included. The remaining
shared libraries come from the operating system. On Ubuntu 22.04, install:

  sudo apt-get install libsdl2-2.0-0 libvpx7 libopenal1 libsndfile1 \
    libmpg123-0 libasound2 libgtk-3-0 libbz2-1.0 libgomp1 zlib1g

Other distributions need compatible versions of these libraries, including
libvpx.so.7 and glibc 2.35 or newer. Vulkan also needs a suitable graphics driver.

SHA256SUMS in the GitHub Release contains checksums for all five archives,
including the Windows PDB download.
Project information and licensing details are in README.md and licenses/.
