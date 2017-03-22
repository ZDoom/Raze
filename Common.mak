
# OS package maintainers: Please try invoking make with PACKAGE_REPOSITORY=1 to see if that meets your needs before patching out our optimizations entirely.
PACKAGE_REPOSITORY ?= 0

# Use colored output. Disable for build system debugging.
PRETTY_OUTPUT ?= 1

# Tools
CROSS=
ifneq ($(CROSS),)
    undefine CC
    undefine CXX
    undefine AR
    undefine RC
    undefine RANLIB
    undefine STRIP
endif

CC=$(CROSS)gcc
CXX=$(CROSS)g++
AR=$(CROSS)ar
RC=$(CROSS)windres
RANLIB=$(CROSS)ranlib
STRIP=$(CROSS)strip
AS=nasm
PKG_CONFIG=pkg-config

NULLSTREAM = /dev/null
DONT_PRINT = > $(NULLSTREAM) 2>&1
DONT_PRINT_STDERR = 2> $(NULLSTREAM)
DONT_FAIL = ; exit 0

# Override defaults that absolutely will not work.
ifeq ($(CC),cc)
    override CC=gcc
endif
ifeq ($(AS),as)
    override AS=nasm
endif

COBJC=$(CC) -x objective-c
COBJCXX=$(CXX) -x objective-c++
L_CC=$(CC)
L_CXX=$(CXX)

CCFULLPATH=$(CC)

ifeq ($(PLATFORM),WII)
    ifeq ($(strip $(DEVKITPPC)),)
        $(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
    endif

    include $(DEVKITPPC)/wii_rules

    CCFULLPATH=$(DEVKITPPC)/bin/$(CC)

    CROSS=powerpc-eabi-
    RANLIB=powerpc-eabi-ranlib
    STRIP=powerpc-eabi-strip
    ELF2DOL=elf2dol
    DOLSUFFIX=.dol
endif

CLANG?=0

CLANG_POTENTIAL_VERSION := $(shell $(CCFULLPATH) --version)

ifeq ($(findstring clang,$(CC)),clang)
    override CLANG=1
    CLANGNAME:=$(CC)
else
    CLANGNAME:=clang
endif
# detect clang symlinked as gcc, as in OS X
ifeq ($(findstring clang,$(CLANG_POTENTIAL_VERSION)),clang)
    override CLANG=1
endif

ifneq (0,$(CLANG))
    override CC=$(CLANGNAME) -x c
    override CXX=$(CLANGNAME) -x c++
    override COBJC=$(CLANGNAME) -x objective-c
    override COBJCXX=$(CLANGNAME) -x objective-c++
    override L_CC=$(CLANGNAME)
    override L_CXX=$(CLANGNAME)
endif

# GCC version, for conditional selection of flags.
ifndef GCC_MAJOR
    GCC_MAJOR    := $(shell $(CCFULLPATH) -dumpversion 2>&1 | cut -d'.' -f1)
endif
ifeq ($(GCC_MAJOR),)
    GCC_MAJOR    := 4
endif
ifndef GCC_MINOR
    GCC_MINOR    := $(shell $(CCFULLPATH) -dumpversion 2>&1 | cut -d'.' -f2)
endif
ifeq ($(GCC_MINOR),)
    GCC_MINOR    := 8
endif

# Detect the platform if it wasn't explicitly given to us from
# the outside world. This allows cross-compilation by overriding
# CC and giving us PLATFORM specifically.
#
ifndef HOSTPLATFORM
    HOSTPLATFORM=UNKNOWN
    ifeq ($(findstring Windows,$(OS)),Windows)
        HOSTPLATFORM=WINDOWS
    else
        uname:=$(strip $(shell uname -s))
        ifeq ($(findstring Linux,$(uname)),Linux)
            HOSTPLATFORM=LINUX
        else ifeq ($(findstring BSD,$(uname)),BSD)
            HOSTPLATFORM=BSD
        else ifeq ($(findstring MINGW,$(uname)),MINGW)
            HOSTPLATFORM=WINDOWS
        else ifeq ($(findstring MSYS,$(uname)),MSYS)
            HOSTPLATFORM=WINDOWS
        else ifeq ($(findstring Darwin,$(uname)),Darwin)
            HOSTPLATFORM=DARWIN
        else ifeq ($(findstring BeOS,$(uname)),BeOS)
            HOSTPLATFORM=BEOS
        else ifeq ($(findstring skyos,$(uname)),skyos)
            HOSTPLATFORM=SKYOS
        else ifeq ($(findstring QNX,$(uname)),QNX)
            HOSTPLATFORM=QNX
        else ifeq ($(findstring SunOS,$(uname)),SunOS)
            HOSTPLATFORM=SUNOS
        else ifeq ($(findstring syllable,$(uname)),syllable)
            HOSTPLATFORM=SYLLABLE
        endif
    endif
endif
ifndef PLATFORM
    PLATFORM=$(HOSTPLATFORM)
endif

ifndef SUBPLATFORM
    SUBPLATFORM=
    ifeq ($(PLATFORM),LINUX)
        SUBPLATFORM=LINUX
    endif
    ifeq ($(PLATFORM),DINGOO)
        SUBPLATFORM=LINUX
        CROSS=mipsel-linux-
    endif
    ifeq ($(PLATFORM),GCW)
        SUBPLATFORM=LINUX
        CROSS=mipsel-linux-
    endif
    ifeq ($(PLATFORM),CAANOO)
        SUBPLATFORM=LINUX
    endif
endif

# Binary suffix override:
EXESUFFIX_OVERRIDE ?=

# Are we running from synthesis?
SYNTHESIS ?= 0

# Mac OS X Frameworks location
# Like above, use absolute paths.
APPLE_FRAMEWORKS ?=/Library/Frameworks

# Engine options
#  USE_OPENGL     - enables basic OpenGL Polymost renderer
#  POLYMER        - enables fancy Polymer renderer
#  NOASM          - disables the use of inline assembly pragmas
#  LINKED_GTK     - enables compile-time linkage to GTK
#
APPNAME=
APPBASENAME=
STANDALONE ?= 0
POLYMER = 1
USE_OPENGL = 1
NOASM = 0
LINKED_GTK = 0
BUILD32_ON_64 ?= 0
USE_LIBPNG ?= 1
USE_LIBVPX ?= 1
HAVE_VORBIS ?= 1
HAVE_FLAC ?= 1
ifeq ($(PLATFORM),WINDOWS)
    HAVE_XMP ?= 1
else
    HAVE_XMP ?= 0
endif
NETCODE ?= 1
STARTUP_WINDOW ?= 1
SIMPLE_MENU ?= 0

LUNATIC ?= 0
USE_LUAJIT_2_1 ?= 0

# EXPERIMENTAL, unfinished x86_64 assembly routines. DO NOT ENABLE.
USE_ASM64 ?= 0

ifeq (0,$(USE_OPENGL))
    override POLYMER = 0
    override USE_LIBVPX = 0
endif

# Debugging/Build options
#  CPLUSPLUS - 1 = enable C++ building
#  RELEASE - 1 = no debugging
#  DEBUGANYWAY:
#    1 = Include debug symbols even when generating release code.
#    2 = Also enable sanitizers with Clang. On the C side, make 'sprite' etc. be real arrays.
#  DISABLEINLINING - 1 = compile inline functions as extern __fastcall instead of static inline
#  KRANDDEBUG - 1 = include logging of krand() calls for debugging the demo system
#  MEMMAP - 1 = produce .memmap file when linking
#  EFENCE  - 1 = compile with Electric Fence for malloc() debugging
#  OPTLEVEL - 0..3 = GCC optimization strategy
#  LTO - 1 = enable link-time optimization, for GCC 4.5 and up
#
CPLUSPLUS?=1
RELEASE?=1
DEBUGANYWAY?=0
KRANDDEBUG?=0
MEMMAP?=0
DISABLEINLINING?=0
EFENCE?=0
DMALLOC?=0
PROFILER?=0
MUDFLAP?=0

# Make allocache() a wrapper around malloc()? Useful for debugging
# allocache()-allocated memory accesses with e.g. Valgrind.
# For debugging with Valgrind + GDB, see
# http://valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.gdbserver
ALLOCACHE_AS_MALLOC?=0

# Select the default optimization level for release and debug builds.
ifeq ($(RELEASE),0)
    OPTLEVEL?=0
else
    OPTLEVEL?=2
endif

ifeq ($(RELEASE),0)
    override STRIP=
endif
ifneq ($(DEBUGANYWAY),0)
    override STRIP=
endif

ifneq ($(LUNATIC),0)
    # FIXME: Lunatic builds with LTO don't start up properly as the required
    # symbol names are apparently not exported.
    override LTO=0
endif

ifndef LTO
    LTO=1
    ifneq (0,$(CLANG))
        ifeq ($(PLATFORM), WINDOWS)
            LTO=0
        endif
    endif
endif

COMMONFLAGS=
COMPILERFLAGS=

ifeq ($(PACKAGE_REPOSITORY),0)
    COMMONFLAGS += $(OPTIMIZATIONS)
endif

OPTIMIZATIONS=-O$(OPTLEVEL) $(OPTOPT)

DEBUGFLAG=-g
ifeq (0,$(CLANG))
    ifneq ($(PLATFORM),WII)
        DEBUGFLAG=-ggdb -fno-omit-frame-pointer
    endif
endif
ifneq ($(RELEASE)$(DEBUGANYWAY),10)
    # debug build or DEBUGANYWAY=1 --> -g flag
    OPTIMIZATIONS += $(DEBUGFLAG)
endif

CSTD:=-std=gnu99
CONLYFLAGS=$(CSTD) -Wimplicit -Werror-implicit-function-declaration
CXXSTD:=-std=gnu++03
CXXONLYFLAGS=$(CXXSTD) -fno-exceptions -fno-rtti
ASFORMAT=elf$(BITS)
ASFLAGS=-s -f $(ASFORMAT) #-g
LINKERFLAGS=
L_CXXONLYFLAGS=
LIBS=-lm
GUI_LIBS=
LIBDIRS=

ifeq (1,$(strip $(shell expr $(GCC_MAJOR) \>= 4)))
    F_NO_STACK_PROTECTOR := -fno-stack-protector
    # there are some link-time issues with stack protectors, so make it possible to override
    F_STACK_PROTECTOR_ALL ?= -fstack-protector-all
    ifeq (0,$(CLANG))
        F_JUMP_TABLES := -fjump-tables
    endif
    M_TUNE_GENERIC := -mtune=generic
    M_STACKREALIGN := -mstackrealign
endif

W_STRICT_OVERFLOW := -Wno-strict-overflow

ifeq ($(HOSTPLATFORM),WINDOWS)
# MSYS2 lets you create files named NUL but has a /dev/null. Go figure.
    ifeq (,$(wildcard /dev/null))
        NULLSTREAM = NUL
    endif
endif

# Detect machine architecture
BITS=32

ifeq ($(PLATFORM),WINDOWS)
    ifndef COMPILERTARGET
        COMPILERTARGET:=$(strip $(shell $(CC) -dumpmachine))
    endif

    IMPLICIT_ARCH=i386
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        IMPLICIT_ARCH=x86_64
        BITS=64
    endif

    WINLIB?=/$(BITS)
else ifeq ($(PLATFORM),WII)
    IMPLICIT_ARCH=ppc
else
    ifneq ($(ARCH),)
        override ARCH:=$(subst i486,i386,$(subst i586,i386,$(subst i686,i386,$(strip $(ARCH)))))
        IMPLICIT_ARCH=$(ARCH)
    else
        IMPLICIT_ARCH:=$(subst i486,i386,$(subst i586,i386,$(subst i686,i386,$(strip $(shell uname -m)))))
    endif

    ifeq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        BITS=64
    endif
endif

ifeq ($(PLATFORM),DARWIN)
    ifndef DARWINVERSION
        DARWINVERSION:=$(strip $(shell uname -r | cut -d . -f 1))
    endif

    DARWIN9 ?= 0
    DARWIN10 ?= 0
    ifeq (1,$(strip $(shell expr $(DARWINVERSION) \< 10)))
        override DARWIN9 := 1
    endif
    ifeq (1,$(strip $(shell expr $(DARWINVERSION) \< 11)))
        override DARWIN10 := 1
    endif

#    COMMONFLAGS    += -fno-leading-underscore

    ifeq (1,$(DARWIN9))
        F_JUMP_TABLES :=
        W_STRICT_OVERFLOW :=
    endif

    ifneq ($(ARCH),)
        ifneq ($(findstring -arch,$(ARCH)),-arch)
            override ARCH := -arch $(ARCH)
        endif
    endif

    ifeq (1,$(BUILD32_ON_64))
        COMMONFLAGS += $(F_NO_STACK_PROTECTOR)
        ifeq ($(ARCH),)
            override ARCH:=-arch i386
        endif
    else
        ifeq ($(findstring ppc,$(IMPLICIT_ARCH)),ppc)
            COMMONFLAGS += $(F_NO_STACK_PROTECTOR)
        endif
    endif

    COMMONFLAGS += $(ARCH)
endif


ifneq (0,$(RELEASE))
    # Debugging disabled
    COMMONFLAGS += $(F_NO_STACK_PROTECTOR)
else
    # Debugging enabled
    ifneq (0,$(KRANDDEBUG))
        COMMONFLAGS += -fno-inline -fno-inline-functions -fno-inline-functions-called-once
    endif
endif

ifndef OPTOPT
    ifeq ($(findstring x86_64, $(IMPLICIT_ARCH)),x86_64)
        ifeq ($(findstring x86_64h, $(IMPLICIT_ARCH)),x86_64h)
            OPTOPT=-march=haswell -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mpclmul -mavx -mrdrnd -mf16c -mfsgsbase -mavx2 -maes -mfma -mbmi -mbmi2
            # -mcrc32 -mmovbe
        else
            ifeq ($(PLATFORM),DARWIN)
                OPTOPT=-march=core2 -mmmx -msse -msse2 -msse3 -mssse3
            endif
        endif
    endif
    ifeq ($(findstring i386, $(IMPLICIT_ARCH)),i386)
        ifeq ($(PLATFORM),DARWIN)
            OPTOPT=-march=nocona -mmmx -msse -msse2 -msse3
        else
            OPTOPT=-march=pentium3 $(M_TUNE_GENERIC) -mmmx
            # -msse2 -mfpmath=sse,387 -malign-double $(M_STACKREALIGN)
        endif
    endif
endif

ifneq (0,$(KRANDDEBUG))
    RELEASE=0
endif
ifneq (0,$(PROFILER))
    # XXX: Why?
    DEBUGANYWAY=1
endif


ifeq ($(PLATFORM),WII)
    override USE_LIBVPX = 0
    override NETCODE = 0
    override HAVE_FLAC = 0
    override HAVE_XMP = 0
endif
ifeq ($(PLATFORM),GCW)
    override USE_LIBVPX = 0
endif
ifeq ($(PLATFORM),DINGOO)
    override USE_LIBVPX = 0
endif

ifneq (0,$(USE_LIBVPX))
    # On Windows, we link statically to libvpx
    LIBS+= -lvpx
endif


ifneq ($(ALLOCACHE_AS_MALLOC),0)
    COMPILERFLAGS += -DDEBUG_ALLOCACHE_AS_MALLOC
endif

# See http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
# for a list of possible UBSan options.
# Clang 3.2 does only supports -fsanitize=address for the AddressSanitizer
CLANG_DEBUG_FLAGS := -fsanitize=address -fsanitize=bounds,enum,float-cast-overflow,object-size
#CLANG_DEBUG_FLAGS := $(CLANG_DEBUG_FLAGS),signed-integer-overflow
#CLANG_DEBUG_FLAGS := $(CLANG_DEBUG_FLAGS),unsigned-integer-overflow
#CLANG_DEBUG_FLAGS := $(CLANG_DEBUG_FLAGS) -fsanitize-undefined-trap-on-error

ifneq (0,$(RELEASE))
    ## Debugging disabled

    ifeq (0,$(CLANG))
        COMMONFLAGS += -funswitch-loops
        ifeq (1,$(strip $(shell expr $(GCC_MAJOR) \< 4)))
            override LTO=0
        endif
        ifeq (1,$(strip $(shell expr $(GCC_MAJOR) = 4)))
            ifeq ($(PLATFORM),WII)
                ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 8)))
                    override LTO=0
                endif
            else
                ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 6)))
                    override LTO=0
                endif
            endif
        endif
    endif

    ifeq (0,$(DEBUGANYWAY))
        COMMONFLAGS += -fomit-frame-pointer
        COMPILERFLAGS += -DNDEBUG
    else
        # Our $(DEBUGANYWAY) -> DEBUGGINGAIDS #define
        COMPILERFLAGS += -DDEBUGGINGAIDS=$(DEBUGANYWAY)
        ifneq (0,$(CLANG))
            ifeq (2,$(DEBUGANYWAY))
                COMMONFLAGS += $(CLANG_DEBUG_FLAGS)
            endif
        endif
    endif

    ifneq (0,$(LTO))
        COMPILERFLAGS += -DUSING_LTO
        COMMONFLAGS += -flto
    endif
else
    ## Debugging enabled

    # Our $(DEBUGANYWAY) -> DEBUGGINGAIDS #define
    COMPILERFLAGS += -DDEBUGGINGAIDS=$(DEBUGANYWAY)
    ifneq (0,$(CLANG))
        ifeq (2,$(DEBUGANYWAY))
            COMMONFLAGS += $(CLANG_DEBUG_FLAGS)
        endif
    endif

    ifeq ($(SUBPLATFORM),LINUX)
        LIBS+=-rdynamic
    endif

    ifneq (0,$(MUDFLAP))
        LIBS+= -lmudflapth
        COMMONFLAGS += -fmudflapth
    endif
    ifneq (0,$(PROFILER))
        ifneq ($(PLATFORM),DARWIN)
            LIBS+= -lprofiler
        endif
        COMMONFLAGS += -pg
    endif
    ifneq (0,$(KRANDDEBUG))
        COMPILERFLAGS += -DKRANDDEBUG=1
    endif
endif

W_UNINITIALIZED := -Wuninitialized
W_GCC_4_1 := -Wno-attributes
W_GCC_4_2 := $(W_STRICT_OVERFLOW)
W_GCC_4_4 := -Wno-unused-result
W_GCC_4_5 := -Wlogical-op -Wcast-qual

CWARNS = -W -Wall \
    -Wextra \
    -Wpointer-arith \
    -Wno-char-subscripts \
    -Wno-missing-braces \
    -Wwrite-strings \
    $(W_UNINITIALIZED) \
    $(W_GCC_4_1) \
    $(W_GCC_4_2) \
    $(W_GCC_4_4) \
    $(W_GCC_4_5) \
    #-Wstrict-prototypes \
    #-Waggregate-return \
    #-Wcast-align \
    #-Waddress

ifneq (0,$(CLANG))
    CWARNS+= -Wno-unused-value -Wno-parentheses -Wno-unknown-warning-option
else
    ifeq (1,$(strip $(shell expr $(GCC_MAJOR) \< 4)))
        W_GCC_4_5 :=
        W_GCC_4_4 :=
        ifeq (0,$(OPTLEVEL))
            W_UNINITIALIZED :=
        endif
        W_GCC_4_2 :=
        W_GCC_4_1 :=
    endif

    ifeq (1,$(strip $(shell expr $(GCC_MAJOR) = 4)))
        ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 5)))
            W_GCC_4_5 :=
            ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 4)))
                W_GCC_4_4 :=
                ifeq (0,$(OPTLEVEL))
                    W_UNINITIALIZED :=
                endif
                ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 2)))
                    W_GCC_4_2 :=
                    ifeq (1,$(strip $(shell expr $(GCC_MINOR) \< 1)))
                        W_GCC_4_1 :=
                    endif
                endif
            endif
        endif
    endif
endif

COMMONFLAGS+= -funsigned-char -fno-strict-aliasing $(F_JUMP_TABLES)

COMPILERFLAGS+= -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0

ifeq (0,$(NETCODE))
    COMPILERFLAGS+= -DNETCODE_DISABLE
endif


#### Lunatic development
# LuaJIT standalone interpreter executable:
LUAJIT:=luajit
# Options to "luajit -b" for synthesis. Since it runs on Linux, we need to tell
# the native LuaJIT to emit PE object files.
ifeq ($(PLATFORM),WINDOWS)
    LUAJIT_BCOPTS := -o windows
    ifeq (32,$(BITS))
        LUAJIT_BCOPTS += -a x86
    endif
    ifeq (64,$(BITS))
        LUAJIT_BCOPTS += -a x64
    endif
endif

ifneq ($(LUNATIC),0)
    COMPILERFLAGS+= -Isource/duke3d/src/lunatic -DLUNATIC
    ifneq ($(USE_LUAJIT_2_1),0)
        COMPILERFLAGS+= -DUSE_LUAJIT_2_1
    endif

    # Determine size of _defs*.lua bytecode once.
    ifndef DEFS_BC_SIZE
        DEFS_BC_SIZE := $(shell $(LUAJIT) -bg -t h source/duke3d/src/lunatic/_defs_game.lua -)
        DEFS_BC_SIZE := $(word 3, $(DEFS_BC_SIZE))
    endif
    ifndef DEFS_M32_BC_SIZE
        DEFS_M32_BC_SIZE := $(shell $(LUAJIT) -bg -t h source/duke3d/src/lunatic/_defs_editor.lua -)
        DEFS_M32_BC_SIZE := $(word 3, $(DEFS_M32_BC_SIZE))
    endif
    COMPILERFLAGS+= -DLUNATIC_DEFS_BC_SIZE=$(DEFS_BC_SIZE) -DLUNATIC_DEFS_M32_BC_SIZE=$(DEFS_M32_BC_SIZE)

    ifeq ($(PLATFORM),WINDOWS)
        LIBS+= -lluajit
    else
        LIBS+= -lluajit-5.1
    endif
endif

####

ifneq (0,$(DISABLEINLINING))
    COMPILERFLAGS+= -DDISABLE_INLINING
endif

# This should come from the environment:
ifdef EDUKE32_MY_DEVELOPER_ID
    COMPILERFLAGS+= -DMY_DEVELOPER_ID=$(EDUKE32_MY_DEVELOPER_ID)
endif

ifneq (0,$(USE_LIBPNG))
    COMPILERFLAGS+= -DUSE_LIBPNG
endif
ifneq (0,$(USE_LIBVPX))
    COMPILERFLAGS+= -DUSE_LIBVPX
endif

ifneq (0,$(STARTUP_WINDOW))
    COMPILERFLAGS+= -DSTARTUP_WINDOW
endif

ifneq (0,$(SIMPLE_MENU))
    COMPILERFLAGS+= -DEDUKE32_SIMPLE_MENU
endif

ifneq (0,$(HAVE_VORBIS))
    COMPILERFLAGS+= -DHAVE_VORBIS
endif
ifneq (0,$(HAVE_FLAC))
    COMPILERFLAGS+= -DHAVE_FLAC
endif
ifneq (0,$(HAVE_XMP))
    COMPILERFLAGS+= -DHAVE_XMP
endif
ifneq (0,$(EFENCE))
    LIBS+= -lefence
    COMPILERFLAGS+= -DEFENCE
endif
ifneq (0,$(DMALLOC))
    LIBS+= -ldmalloc
    COMPILERFLAGS+= -DDMALLOC
endif
ifneq (0,$(STANDALONE))
    COMPILERFLAGS+= -DEDUKE32_STANDALONE
endif

ifneq (,$(APPNAME))
    COMPILERFLAGS+= -DAPPNAME=\"$(APPNAME)\"
endif
ifneq (,$(APPBASENAME))
    COMPILERFLAGS+= -DAPPBASENAME=\"$(APPBASENAME)\"
endif

# may be overridden
EXESUFFIX=
DLLSUFFIX=.so


SDL_TARGET ?= 2
SDL_FRAMEWORK ?= 0

ifeq (1,$(strip $(shell expr $(GCC_MAJOR) \>= 4)))
    L_SSP := -lssp
endif

# ifeq (1,$(strip $(shell expr $(GCC_MAJOR) \>= 5)))
#     ifneq (0,$(LTO))
#         COMMONFLAGS += -fno-lto-odr-type-merging
#     endif
# endif

ifeq ($(SUBPLATFORM),LINUX)
    GTKCOMPAT32=0

    ifeq ($(PLATFORM),GCW)
        override USE_OPENGL=0
        override NOASM=1
    endif
    ifeq ($(PLATFORM),DINGOO)
        override USE_OPENGL=0
        override NOASM=1
    endif

    ifneq ($(findstring i386,$(IMPLICIT_ARCH)),i386)
        ifeq (1,$(BUILD32_ON_64))
            # On my 64bit Gentoo these are the 32bit emulation libs
            LIBDIRS+= -L/emul/linux/x86/usr/lib
            COMMONFLAGS+= -m32
            # Override WITHOUT_GTK=0
            GTKCOMPAT32=1
        endif
    endif

    LIBS+= -lrt
endif
ifeq ($(PLATFORM),DARWIN)
    COMPILERFLAGS+= -DUNDERSCORES
    ASFORMAT=macho
    ASFLAGS+= -DUNDERSCORES

    ifeq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        ASFORMAT+=64
        override NOASM=1
    endif

    # LIBDIRS+= -Lplatform/Apple/lib
    # COMPILERFLAGS+= -Iplatform/Apple/include

    ifneq ($(shell port --version &>/dev/null; echo $$?),127)
        LIBDIRS+= -L/opt/local/lib
        COMPILERFLAGS+= -I/opt/local/include
    endif
    ifneq ($(shell brew --version &>/dev/null; echo $$?),127)
        LIBDIRS+= -L/usr/local/lib
        COMPILERFLAGS+= -I/usr/local/include
    endif
    ifneq ($(shell fink --version &>/dev/null; echo $$?),127)
        LIBDIRS+= -L/sw/lib
        COMPILERFLAGS+= -I/sw/include
    endif

    DLLSUFFIX=.dylib
    GTKCOMPAT32    = 0
    WITHOUT_GTK   ?= 1

    ifeq (1,$(DARWIN9))
        COMPILERFLAGS += -DDARWIN9
    endif

    ifneq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        LINKERFLAGS += -read_only_relocs suppress
    endif
endif
ifeq ($(PLATFORM),WINDOWS)
    COMPILERFLAGS+= -DUNDERSCORES -Iplatform/Windows/include
    LINKERFLAGS+= -static-libgcc
    ifeq (0,$(CLANG))
        L_CXXONLYFLAGS += -static-libstdc++
    endif
    ASFORMAT=win$(BITS)
    ASFLAGS+= -DUNDERSCORES

    # Windows types can be SDL or WIN
    RENDERTYPE?=SDL
    MIXERTYPE?=WIN
    ifneq ($(RENDERTYPE),SDL)
        ifeq ($(MIXERTYPE),SDL)
            override MIXERTYPE:=WIN
        endif
    endif

    WITHOUT_GTK?=1
    EXESUFFIX=.exe
    DLLSUFFIX=.dll
    LIBDIRS+= -Lplatform/Windows/lib$(WINLIB)
    LIBS+= -Wl,--enable-auto-import -lmingwex -lgdi32 -lcomctl32 -lwinmm $(L_SSP) -lwsock32 -lws2_32 -lshlwapi
    ifeq (0,$(CLANG))
        GUI_LIBS += -mwindows
    endif
    #-lshfolder
else
    RENDERTYPE?=SDL
    MIXERTYPE?=SDL
endif
ifeq ($(PLATFORM),BSD)
    COMPILERFLAGS+= -I/usr/local/include
endif
ifeq ($(PLATFORM),BEOS)
    override NOASM=1
endif
ifeq ($(PLATFORM),SKYOS)
    EXESUFFIX=.app
    override NOASM=1
    COMPILERFLAGS+= -DUNDERSCORES -I/boot/programs/sdk/include/sdl
    SDLCONFIG=
    LIBS+= -lSDL -lnet
endif
ifeq ($(PLATFORM),WII)
    EXESUFFIX=.elf
    override USE_OPENGL=0
    override POLYMER=0
    override NOASM=1
    override WITHOUT_GTK=1
    # -msdata=eabi
    COMMONFLAGS+= -g -mtune=750 -meabi -mhard-float
    COMPILERFLAGS+= -DGEKKO -D__POWERPC__ -I$(LIBOGC_INC) -I$(PORTLIBS)/include -Iplatform/Wii/include
    SDLCONFIG=
    SDL_TARGET=1
    LIBDIRS += -L$(LIBOGC_LIB) -L$(PORTLIBS)/lib -Lplatform/Wii/lib
endif
ifeq ($(PLATFORM),QNX)
    override USE_OPENGL=0
    override NOASM=1
    LIBS+= -lsocket
endif
ifeq ($(PLATFORM),SUNOS)
    override USE_OPENGL=0
    override NOASM=1
    LIBS+= -lsocket -lnsl
endif
ifeq ($(PLATFORM),SYLLABLE)
    override USE_OPENGL=0
    override NOASM=1
endif
ifeq ($(PLATFORM),GCW)
    COMPILERFLAGS += -D__OPENDINGUX__
endif
ifeq ($(PLATFORM),DINGOO)
    COMPILERFLAGS += -D__OPENDINGUX__
endif

ifneq ($(findstring i386,$(IMPLICIT_ARCH)),i386)
    override NOASM=1
endif

ifneq ($(EXESUFFIX_OVERRIDE),)
    EXESUFFIX=$(EXESUFFIX_OVERRIDE)
endif

ifeq ($(RENDERTYPE),SDL)
    ifeq ($(SDL_TARGET),2)
        SDLCONFIG ?= sdl2-config
        SDLNAME ?= SDL2
    endif
    ifeq ($(SDL_TARGET),1)
        SDLCONFIG ?= sdl-config
        SDLNAME ?= SDL
        ifeq (0,$(RELEASE))
            COMPILERFLAGS += -DNOSDLPARACHUTE
        endif
    endif

    ifneq ($(strip $(SDLCONFIG)),)
        ifeq ($(strip $(shell $(SDLCONFIG) --version $(DONT_PRINT_STDERR))),)
            override SDLCONFIG =
        endif
    endif

    COMPILERFLAGS += -DSDL_TARGET=$(SDL_TARGET)

    ifneq ($(SDL_FRAMEWORK),0)
        ifeq ($(PLATFORM),DARWIN)
            LIBDIRS += -F$(APPLE_FRAMEWORKS)
            ifeq ($(MIXERTYPE),SDL)
                COMPILERFLAGS += -I$(APPLE_FRAMEWORKS)/$(SDLNAME)_mixer.framework/Headers
                LIBS += -Wl,-framework,$(SDLNAME)_mixer
            endif
            COMPILERFLAGS += -I$(APPLE_FRAMEWORKS)/$(SDLNAME).framework/Headers
            LIBS += -Wl,-framework,$(SDLNAME) -Wl,-rpath -Wl,"@loader_path/../Frameworks"
        endif
    else
        ifeq ($(MIXERTYPE),SDL)
            LIBS += -l$(SDLNAME)_mixer
        endif
        ifneq ($(SDLCONFIG),)
            SDLCONFIG_CFLAGS := $(strip $(subst -Dmain=SDL_main,,$(shell $(SDLCONFIG) --cflags)))
            SDLCONFIG_LIBS := $(strip $(subst -mwindows,,$(shell $(SDLCONFIG) --libs)))

            COMMONFLAGS += $(SDLCONFIG_CFLAGS)
            LIBS += $(SDLCONFIG_LIBS)
        else
            ifeq ($(SDL_TARGET),1)
                COMPILERFLAGS += -D_GNU_SOURCE=1
            endif
            COMPILERFLAGS += -D_REENTRANT -DSDL_USEFOLDER
            LIBS+= -l$(SDLNAME)
        endif
    endif

    ifeq (1,$(WITHOUT_GTK))
        HAVE_GTK2=0
    else
        ifneq (No,$(shell $(PKG_CONFIG) --exists gtk+-2.0 || echo No))
            HAVE_GTK2=1
            # On my 64bit Gentoo box I have Cairo enabled which means the libs list includes
            # -lpangocairo-1.0 and -lcairo, however the 32bit compatibility libraries don't
            # include cairo, so we need to filter out those -l switches in order to link
            ifneq ($(LINKED_GTK),0)
                ifeq ($(GTKCOMPAT32),1)
                    LIBS+= $(shell $(PKG_CONFIG) --libs gtk+-2.0 | sed 's/\s-l\(pango\)\{0,1\}cairo\S*\s/ /g')
                else
                    LIBS+= $(shell $(PKG_CONFIG) --libs gtk+-2.0)
                endif
            endif
            COMPILERFLAGS += -DHAVE_GTK2 $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
        else
            HAVE_GTK2=0
        endif
    endif

    ifeq ($(PLATFORM),WINDOWS)
        LIBS += -ldxguid_sdl -lmingw32 -lgdi32 -limm32 -lole32 -loleaut32 -lwinmm -lversion
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    LIBS+= -ldxguid
endif

ifeq ($(PLATFORM),WII)
    LIBS+= -laesnd_tueidj -lpng -lfat -lwiiuse -lbte -logc -lm -lwiikeyboard
endif

COMPILERFLAGS+= -DRENDERTYPE$(RENDERTYPE)=1 -DMIXERTYPE$(MIXERTYPE)=1

ifneq (0,$(USE_OPENGL))
    COMPILERFLAGS+= -DUSE_OPENGL
endif
ifneq (0,$(NOASM))
    COMPILERFLAGS+= -DNOASM
endif
ifneq (0,$(USE_ASM64))
    COMPILERFLAGS+= -DUSE_ASM64
endif
ifneq (0,$(LINKED_GTK))
    COMPILERFLAGS+= -DLINKED_GTK
endif

ifneq (0,$(POLYMER))
 ifneq (0,$(USE_OPENGL))
  COMPILERFLAGS+= -DPOLYMER
 endif
endif


ifneq ($(PLATFORM),WINDOWS)
    ifneq ($(PLATFORM),WII)
        ifneq ($(PLATFORM),BSD)
            LIBS+= -ldl
        endif
        ifneq ($(PLATFORM),DARWIN)
            LIBS+= -pthread
        endif
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    ifneq ($(USE_LIBPNG),0)
        LIBS+= -lpng_mini -lz_mini
    endif
else
    ifeq ($(PLATFORM),DARWIN)
        ifneq ($(USE_LIBPNG),0)
            LIBS+= -lpng # -lz
        endif
    else
        ifneq ($(USE_LIBPNG),0)
            LIBS+= -lpng -lz
        endif
    endif
endif


ifeq ($(PLATFORM),WINDOWS)
    ifneq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        LINKERFLAGS+= -Wl,--large-address-aware
    endif
endif

ifneq (0,$(MEMMAP))
ifeq ($(PLATFORM),DARWIN)
    LINKERFLAGS+=-Wl,-map -Wl,$@.memmap
else
    LINKERFLAGS+=-Wl,-Map=$@.memmap
endif
endif

ifneq (0,$(PROFILER))
    LINKERFLAGS+=-pg
endif
ifeq ($(PLATFORM),WII)
    LINKERFLAGS+= -mrvl -meabi -mhard-float -Wl,--gc-sections
    # -msdata=eabi
endif

# Detect version control revision, if applicable
ifeq (,$(VC_REV))
    ifneq (,$(wildcard EDUKE32_REVISION))
        VC_REV    := $(shell cat EDUKE32_REVISION)
    endif
endif
ifeq (,$(VC_REV))
    VC_REV    := $(shell svn info 2>&1 | grep Revision | cut -d' ' -f2)
endif
ifeq (,$(VC_REV))
    VC_REV    := $(shell git svn info 2>&1 | grep Revision | cut -d' ' -f2)
endif
ifneq (,$(VC_REV)$(VC_REV_CUSTOM))
    REVFLAG += -DREV="\"r$(VC_REV)$(VC_REV_CUSTOM)\""
endif

COMPILER_C=$(CC) $(CONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS)
COMPILER_CXX=$(CXX) $(CXXONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS)
COMPILER_OBJC=$(COBJC) $(CONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS)
COMPILER_OBJCXX=$(COBJCXX) $(CXXONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS)
LINKER=$(L_CXX) $(CXXONLYFLAGS) $(L_CXXONLYFLAGS) $(COMMONFLAGS) $(LINKERFLAGS)
ifneq ($(CPLUSPLUS),0)
    COMPILER_C=$(COMPILER_CXX)
    COMPILER_OBJC=$(COMPILER_OBJCXX)
endif

ifneq (,$(CUSTOMOPT))
    COMMONFLAGS+= $(CUSTOMOPT)
endif
ifneq (,$(CFLAGS))
    COMMONFLAGS+= $(CFLAGS)
endif
ifneq (,$(CXXFLAGS))
    CXXONLYFLAGS+= $(CXXFLAGS)
endif
ifneq (,$(LDFLAGS))
    LINKERFLAGS+= $(LDFLAGS)
endif

ifeq ($(PRETTY_OUTPUT),1)
RECIPE_IF = if
BUILD_SETTINGS_COMPILER = \033[1;36mcompiler: \033[0;36m\"$(COMPILER_C)\"
BUILD_SETTINGS_ASSEMBLER = \033[1;36massembler: \033[0;36m\"$(AS) $(ASFLAGS)\"
BUILD_SETTINGS_LINKER = \033[1;36mlinker: \033[0;36m\"$(LINKER) $(GUI_LIBS) $(LIBDIRS) $(LIBS)\"
ifeq (0,$(NOASM))
    BUILD_SETTINGS = printf "$(BUILD_SETTINGS_COMPILER)\n$(BUILD_SETTINGS_ASSEMBLER)\n$(BUILD_SETTINGS_LINKER)\033[0m\n"
else
    BUILD_SETTINGS = printf "$(BUILD_SETTINGS_COMPILER)\n$(BUILD_SETTINGS_LINKER)\033[0m\n"
endif
BUILD_STARTED = printf "\033[K\033[1;36mBuild started using:\033[0m\n"; $(BUILD_SETTINGS)
BUILD_ECHOFLAGS = printf "\033[K\033[1;36mEnded compilation in this directory using:\n$(BUILD_SETTINGS_COMPILER)\033[0m\n"
COMPILE_STATUS = printf "\033[K\033[0mBuilding object \033[1m$@\033[0m...\033[0m\r"
COMPILE_OK = printf "\033[K\033[0;32mBuilt object \033[1;32m$@\033[0;32m \033[0m\n"
COMPILE_FAILED = printf "\033[K\033[0;31mFailed building \033[1;31m$@\033[0;31m from\033[0m \033[1;31m$<\033[0;31m!\033[0m\n"; exit 1
RECIPE_RESULT_COMPILE = ; then $(COMPILE_OK); else $(COMPILE_FAILED); fi
ONESTEP_STATUS = printf "\033[K\033[0mBuilding \033[1m$@\033[0m...\033[0m\r"
ONESTEP_OK = printf "\033[K\033[0;32mBuilt \033[1;32m$@\033[0;32m \033[0m\n"
ONESTEP_FAILED = printf "\033[K\033[0;31mFailed building \033[1;31m$@\033[0;31m!\033[0m\n"; exit 1
RECIPE_RESULT_ONESTEP = ; then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
ARCHIVE_STATUS = printf "\033[K\033[0mCreating library archive \033[1m$@\033[0m...\033[0m\r"
ARCHIVE_OK = printf "\033[K\033[0;32mCreated library archive \033[1;32m$@\033[0;32m \033[0m\n"
ARCHIVE_FAILED = printf "\033[K\033[0;31mFailed creating library archive \033[1;31m$@\033[0;31m from\033[0m \033[1;31m$<\033[0;31m!\033[0m\n"; exit 1
RECIPE_RESULT_ARCHIVE = ; then $(ARCHIVE_OK); else $(ARCHIVE_FAILED); fi
LINK_STATUS = printf "\033[K\033[0;0mLinking \033[1m$@\033[0;0m...\033[0m\r"
LINK_OK = printf "\033[K\033[0;32mLinked \033[1;32m$@\033[0;32m \033[0m\n"
LINK_FAILED = printf "\033[K\033[0;31mFailed linking \033[1;31m$@\033[0;31m!\nIf the build options, environment, or system packages have changed, run \'\033[1;31mmake clean\033[0;31m\' and try again.\033[0m\n"; exit 1
RECIPE_RESULT_LINK = ; then $(LINK_OK); else $(LINK_FAILED); fi
else
RECIPE_IF =
BUILD_STARTED =
BUILD_ECHOFLAGS =
COMPILE_STATUS =
COMPILE_OK = true
COMPILE_FAILED = false; exit 1
RECIPE_RESULT_COMPILE =
ONESTEP_STATUS =
ONESTEP_OK = true
ONESTEP_FAILED = false; exit 1
RECIPE_RESULT_ONESTEP =
ARCHIVE_STATUS =
ARCHIVE_OK = true
ARCHIVE_FAILED = false; exit 1
RECIPE_RESULT_ARCHIVE =
LINK_STATUS =
LINK_OK = true
LINK_FAILED = false; exit 1
RECIPE_RESULT_LINK =
endif
