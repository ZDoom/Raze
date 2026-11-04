# Welcome to Raze!

[![Continuous Integration](https://github.com/ZDoom/Raze/actions/workflows/continuous_integration.yml/badge.svg)](https://github.com/ZDoom/Raze/actions/workflows/continuous_integration.yml)

## Raze is a fork of Build engine games backed by GZDoom tech and combines Duke Nukem 3D, Blood, Redneck Rampage, Shadow Warrior and Exhumed/Powerslave in a single package. It is also capable of playing Nam and WW2 GI.

The game modules are based on the following sources:

  * Duke Nukem: JFDuke, EDuke 2.0, World Tour extensions from DukeGDX and some minor fixes from EDuke32.
  * Redneck Rampage: Nuke.YKT's reconstructed source available in the Rednukem Git repo.
  * Blood: NBlood.
  * Shadow Warrior: SWP and VoidSW.
  * Exhumed/Powerslave: PCExhumed, with some enhancements inspired by PowerslaveGDX.

ZDoom, GZDoom Copyright (c) 1998-2022 ZDoom + GZDoom teams, and contributors

Doom Source (c) 1997 id Software, Raven Software, and contributors

EDuke32 and VoidSW Source (c) 2005-2020 EDuke32 teams, and contributors

NBlood source (c) 2019-2020 Nuke.YKT

PCExhumed source (c) 2019-2020 sirlemonhead, Nuke.YKT

BuildGDX (c) 2020

Duke Nukem 3D Source (c) 1996-2003 3D Realms

Shadow Warrior Source (c) 1997-2005 3D Realms

"Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
Ken Silverman's official web site: http://www.advsys.net/ken
See the included license file "BUILDLIC.TXT" for license info.

Please see license files for individual contributor licenses

Special thanks to Coraline of the 3DGE team for allowing us to use her README.md as a template for this one.

### Non-Build code is licensed under the GPL v2
##### https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
---

## How to build Raze

To build Raze, please see the [wiki](https://zdoom.org/wiki/) and see the "Programmer's Corner" on the bottom-right corner of the page to build for your platform - use this repository instead of GZDoom's.

### macOS dependencies

Use ZMusic 1.1.14, matching the CI builds. For builds using the vcpkg toolchain,
these options bundle the MoltenVK runtime and link OpenAL Soft for audio:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DMOLTENVK_VCPKG=ON -DOPENAL_SOFT_VCPKG=ON -DDYN_OPENAL=OFF
cmake --build build --parallel
```

The local vcpkg overlay downloads the official MoltenVK 1.4.2 macOS binary,
verifies its checksum, and bundles it and its license in `raze.app`. It supports
Apple Silicon and Intel Macs running macOS 12 or newer. Vulkan must remain
enabled (`HAVE_VULKAN=ON`, the default). The MoltenVK option is off by default;
builds without it can still use an externally installed Vulkan runtime.

### Automated releases

The [Release workflow](.github/workflows/release.yml) builds Windows x64, Linux
x64, macOS Intel and macOS Apple Silicon packages. Windows uses the native Win32
backend with a 64-bit executable; 32-bit x86 builds are not currently supported.

Push a tag in the form `vMAJOR.MINOR.PATCH` to build and publish a GitHub Release:

```sh
git tag -a v1.2.3 -m "Release v1.2.3"
git push origin v1.2.3
```

Use an unused version number and tag the commit containing the release changes.
All four builds must succeed before the workflow publishes the five archives and
`SHA256SUMS`. Only the publishing job receives write access, using the built-in
`GITHUB_TOKEN`; no release secret is required.

For a test build, run **Actions → Release → Run workflow** on a branch or tag.
Manual runs upload artifacts for seven days without creating a release. Their
filenames use `ci-RUN_NUMBER-RUN_ATTEMPT`. Manual dispatch becomes available once
the workflow is on the repository's default branch.

To test the complete release process from a feature branch, push a tag such as
`v2.0.0-test.1`. Tags ending in `-test.N` build all packages and upload them to a
**draft prerelease**. They never publish a public release or mark it as latest.
Use a new test number for each revised commit. Other prerelease suffixes are
rejected; only `vMAJOR.MINOR.PATCH` tags publish stable releases.

Windows and macOS packages include their runtime libraries, engine resources and
soundfont. macOS builds require macOS 15 or newer and are ad-hoc signed. Linux
packages target Ubuntu 22.04 and include ZMusic; system library requirements and
launch instructions are in the bundled [release README](package/release/README.txt).
ZMusic 1.1.14, vcpkg and GitHub Actions are pinned in the workflow.
Release builds use the pinned vcpkg checkout's dependency versions for both Raze
and ZMusic, preserving the original manifests for normal developer builds.

The Windows build uses optimized `RelWithDebInfo` binaries with full PDBs. A
separate `raze-VERSION-windows-x86_64-pdb.7z` download contains matching symbols
for `raze.exe` and `zmusiclite.dll`, plus codec symbols when available. The PDB
archive is also included in `SHA256SUMS` and in manual workflow artifacts.

# Resources
- https://raze.zdoom.org/ - Home Page
- https://forum.zdoom.org/viewforum.php?f=351 - Forum
- https://raze.zdoom.org/wiki/ - Wiki
- https://dsc.gg/zdoom - Discord Server
- https://docs.google.com/spreadsheets/d/1pvwXEgytkor9SClCiDn4j5AH7FedyXS-ocCbsuQIXDU/edit?usp=sharing - Translation sheet (Google Docs)
