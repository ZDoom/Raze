
# OS package maintainers: Please try invoking make with PACKAGE_REPOSITORY=1 to see if that meets your needs before patching out our optimizations entirely.
PACKAGE_REPOSITORY ?= 0

# Are we running from synthesis?
SYNTHESIS := 0


##### Makefile Swiss army knife

override empty :=
override space := $(empty) $(empty)
override comma := ,


##### Detect platform

ifndef HOSTPLATFORM
    HOSTPLATFORM := UNKNOWN
    ifeq ($(findstring Windows,$(OS)),Windows)
        HOSTPLATFORM := WINDOWS
    else
        uname := $(strip $(shell uname -s))
        ifeq ($(findstring Linux,$(uname)),Linux)
            HOSTPLATFORM := LINUX
        else ifeq ($(findstring BSD,$(uname)),BSD)
            HOSTPLATFORM := BSD
        else ifeq ($(findstring MINGW,$(uname)),MINGW)
            HOSTPLATFORM := WINDOWS
        else ifeq ($(findstring MSYS,$(uname)),MSYS)
            HOSTPLATFORM := WINDOWS
        else ifeq ($(findstring Darwin,$(uname)),Darwin)
            HOSTPLATFORM := DARWIN
        else ifeq ($(findstring BeOS,$(uname)),BeOS)
            HOSTPLATFORM := BEOS
        else ifeq ($(findstring skyos,$(uname)),skyos)
            HOSTPLATFORM := SKYOS
        else ifeq ($(findstring QNX,$(uname)),QNX)
            HOSTPLATFORM := QNX
        else ifeq ($(findstring SunOS,$(uname)),SunOS)
            HOSTPLATFORM := SUNOS
        else ifeq ($(findstring syllable,$(uname)),syllable)
            HOSTPLATFORM := SYLLABLE
        endif
    endif
endif
ifndef PLATFORM
    PLATFORM := $(HOSTPLATFORM)
endif

ifndef SUBPLATFORM
    SUBPLATFORM :=
    ifeq ($(PLATFORM),$(filter $(PLATFORM),LINUX DINGOO GCW CAANOO))
        SUBPLATFORM := LINUX
    endif
endif

ifeq ($(HOSTPLATFORM),DARWIN)
    DARWINVERSION := $(word 1,$(subst ., ,$(strip $(shell uname -r))))

    DARWIN9 := 0
    DARWIN10 := 0
    ifneq (,$(filter 0 1 2 3 4 5 6 7 8 9 10,$(DARWINVERSION)))
        DARWIN10 := 1
        ifneq (,$(filter 0 1 2 3 4 5 6 7 8 9,$(DARWINVERSION)))
            DARWIN9 := 1
        endif
    endif
endif

HOSTEXESUFFIX :=
ifeq ($(HOSTPLATFORM),WINDOWS)
    HOSTEXESUFFIX := .exe
endif

EXESUFFIX :=
DLLSUFFIX := .so
DOLSUFFIX := .dol
ifeq ($(PLATFORM),DARWIN)
    DLLSUFFIX := .dylib
endif
ifeq ($(PLATFORM),WII)
    EXESUFFIX := .elf
endif
ifeq ($(PLATFORM),SKYOS)
    EXESUFFIX := .app
endif
ifeq ($(PLATFORM),WINDOWS)
    EXESUFFIX := .exe
    DLLSUFFIX := .dll
endif


##### Makefile meta-settings

PRETTY_OUTPUT := 1

NULLSTREAM := /dev/null

ifeq ($(HOSTPLATFORM),WINDOWS)
# MSYS2 lets you create files named NUL but has a /dev/null. Go figure.
    ifeq (,$(wildcard /dev/null))
        NULLSTREAM := NUL
    endif
endif

DONT_PRINT := > $(NULLSTREAM) 2>&1
DONT_PRINT_STDERR := 2> $(NULLSTREAM)
DONT_FAIL := ; exit 0

HAVE_SH := 1
# when no sh.exe is found in PATH on Windows, no path is prepended to it
ifeq (sh.exe,$(SHELL))
    HAVE_SH := 0
endif

define LL
    ls -l $1
endef
define MKDIR
    mkdir -p $1
endef
define RM
    rm -f $(filter-out / *,$1)
endef
define RMDIR
    rm -rf $(filter-out / *,$1)
endef

ifeq (0,$(HAVE_SH))
    DONT_FAIL := & rem

    define LL
        dir $(subst /,\,$1)
    endef
    define MKDIR
        if not exist $(subst /,\,$1) mkdir $(subst /,\,$1)
    endef
    define RM
        del /f /q $(subst /,\,$(filter-out / *,$1)) $(DONT_PRINT_STDERR) $(DONT_FAIL)
    endef
    define RMDIR
        rmdir /s /q $(subst /,\,$(filter-out / *,$1)) $(DONT_PRINT_STDERR) $(DONT_FAIL)
    endef

    # if, printf, exit, and ; are unavailable without sh
    PRETTY_OUTPUT := 0
endif


##### Toolchain setup

CROSS :=
CROSS_SUFFIX :=

CCFULLPATH = $(CC)

ifeq ($(PLATFORM),WII)
    ifeq ($(strip $(DEVKITPPC)),)
        $(error "Please set DEVKITPPC in your environment. export DEVKITPPC := <path to>devkitPPC")
    endif

    ifeq ($(HOSTPLATFORM),WINDOWS)
        override DEVKITPRO := $(subst /c/,C:/,$(DEVKITPRO))
        override DEVKITPPC := $(subst /c/,C:/,$(DEVKITPPC))
    endif

    export PATH := $(DEVKITPPC)/bin:$(PATH)

    CROSS := powerpc-eabi-

    CCFULLPATH = $(DEVKITPPC)/bin/$(CC)
endif

CC := $(CROSS)gcc$(CROSS_SUFFIX)
CXX := $(CROSS)g++$(CROSS_SUFFIX)

ifeq ($(PLATFORM),DARWIN)
    CC := $(CROSS)clang$(CROSS_SUFFIX)
    CXX := $(CROSS)clang++$(CROSS_SUFFIX)
endif

COBJC := $(CC) -x objective-c
COBJCXX := $(CXX) -x objective-c++
L_CC := $(CC)
L_CXX := $(CXX)

AR := $(CROSS)ar$(CROSS_SUFFIX)
RC := $(CROSS)windres$(CROSS_SUFFIX)
RANLIB := $(CROSS)ranlib$(CROSS_SUFFIX)
STRIP := $(CROSS)strip$(CROSS_SUFFIX)

AS := nasm

# LuaJIT standalone interpreter executable:
LUAJIT := luajit$(HOSTEXESUFFIX)

PKG_CONFIG := pkg-config

ELF2DOL := elf2dol

# Override defaults that absolutely will not work.
ifeq ($(CC),cc)
    override CC := gcc
endif
ifeq ($(AS),as)
    override AS := nasm
endif

ifeq ($(PLATFORM),$(filter $(PLATFORM),DINGOO GCW))
    CROSS := mipsel-linux-
endif

CLANG := 0
ifeq ($(findstring clang,$(CC) $(MAKECMDGOALS)),clang)
    override CLANG := 1
    CLANGNAME := $(CC)
else
    CLANGNAME := clang
endif
# detect clang symlinked as gcc, as in OS X
CLANG_POTENTIAL_VERSION := $(shell $(CCFULLPATH) --version)
ifeq ($(findstring clang,$(CLANG_POTENTIAL_VERSION)),clang)
    override CLANG := 1
endif

ifneq (0,$(CLANG))
    CLANGXXNAME := $(subst clang,clang++,$(CLANGNAME))
    override CC := $(CLANGNAME) -x c
    override CXX := $(CLANGXXNAME) -x c++
    override COBJC := $(CLANGNAME) -x objective-c
    override COBJCXX := $(CLANGXXNAME) -x objective-c++
    override L_CC := $(CLANGNAME)
    override L_CXX := $(CLANGXXNAME)
endif

GCC_VER :=
ifeq (0,$(CLANG))
    GCC_VER := $(strip $(shell $(CCFULLPATH) -dumpversion 2>&1))
endif
ifeq (,$(GCC_VER))
    GCC_VER := 4.9.0
endif
override GCC_VER_SPLIT := $(subst ., ,$(GCC_VER))
GCC_MAJOR := $(word 1,$(GCC_VER_SPLIT))
GCC_MINOR := $(word 2,$(GCC_VER_SPLIT))

GCC_PREREQ_4 := 1

ifneq (,$(filter 1 2 3,$(GCC_MAJOR)))
    ifeq (0,$(CLANG))
        GCC_PREREQ_4 := 0
        $(error How do you still have an old GCC in $$(CURRENT_YEAR)?)
    endif
endif


##### Detect machine architecture

BITS := 32

ifeq ($(PLATFORM),WINDOWS)
    ifndef COMPILERTARGET
        COMPILERTARGET := $(strip $(shell $(CC) -dumpmachine))
    endif

    IMPLICIT_ARCH := i386
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        IMPLICIT_ARCH := x86_64
        BITS := 64
    endif
else ifeq ($(PLATFORM),WII)
    IMPLICIT_ARCH := ppc
else
    ifneq ($(ARCH),)
        override ARCH := $(subst i486,i386,$(subst i586,i386,$(subst i686,i386,$(strip $(ARCH)))))
        IMPLICIT_ARCH := $(ARCH)
    else
        IMPLICIT_ARCH := $(subst i486,i386,$(subst i586,i386,$(subst i686,i386,$(strip $(shell uname -m)))))
    endif

    ifeq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        BITS := 64
    endif
endif


##### Toggles

#  CPLUSPLUS - 1 := enable C++ building
#  RELEASE - 1 := no debugging
#  FORCEDEBUG:
#    1 := Include debug symbols even when generating release code.
#    2 := Also enable sanitizers with Clang. On the C side, make 'sprite' etc. be real arrays.
#  KRANDDEBUG - 1 := include logging of krand() calls for debugging the demo system
#  MEMMAP - 1 := produce .memmap file when linking
#  OPTLEVEL - 0..3 := GCC optimization strategy
#  LTO - 1 := enable link-time optimization

# Optional overrides for text
APPNAME :=
APPBASENAME :=

# Build toggles
RELEASE := 1
NOASM := 0
# EXPERIMENTAL, unfinished x86_64 assembly routines. DO NOT ENABLE.
USE_ASM64 := 0
MEMMAP := 0
CPLUSPLUS := 1

# Feature toggles
STANDALONE := 0
NETCODE := 1
STARTUP_WINDOW := 1
SIMPLE_MENU := 0
POLYMER := 1
USE_OPENGL := 1
LUNATIC := 0
USE_LUAJIT_2_1 := 0

# Library toggles
HAVE_GTK2 := 1
USE_LIBVPX := 1
HAVE_VORBIS := 1
HAVE_FLAC := 1
HAVE_XMP := 1
RENDERTYPE := SDL
MIXERTYPE := SDL
SDL_TARGET := 2

# Debugging/Build options
FORCEDEBUG := 0
KRANDDEBUG := 0
PROFILER := 0
# Make allocache() a wrapper around malloc()? Useful for debugging
# allocache()-allocated memory accesses with e.g. Valgrind.
# For debugging with Valgrind + GDB, see
# http://valgrind.org/docs/manual/manual-core-adv.html#manual-core-adv.gdbserver
ALLOCACHE_AS_MALLOC := 0


##### Settings overrides and implicit cascades

ifneq (0,$(KRANDDEBUG))
    RELEASE := 0
endif
ifneq (100,$(RELEASE)$(PROFILER)$(ALLOCACHE_AS_MALLOC))
    # so that we have debug symbols
    FORCEDEBUG := 1
endif

ifeq ($(PLATFORM),WINDOWS)
    MIXERTYPE := WIN
    ifneq ($(RENDERTYPE),SDL)
        ifeq ($(MIXERTYPE),SDL)
            override MIXERTYPE := WIN
        endif
    endif
    override HAVE_GTK2 := 0
else ifeq ($(PLATFORM),DARWIN)
    HAVE_GTK2 := 0
else ifeq ($(PLATFORM),WII)
    override USE_OPENGL := 0
    override NETCODE := 0
    override HAVE_GTK2 := 0
    override HAVE_FLAC := 0
    SDL_TARGET := 1
else ifeq ($(PLATFORM),$(filter $(PLATFORM),DINGOO GCW QNX SUNOS SYLLABLE))
    override USE_OPENGL := 0
    override NOASM := 1
else ifeq ($(PLATFORM),$(filter $(PLATFORM),BEOS SKYOS))
    override NOASM := 1
endif

ifneq (i386,$(strip $(IMPLICIT_ARCH)))
    override NOASM := 1
endif

ifeq (0,$(USE_OPENGL))
    override POLYMER := 0
    override USE_LIBVPX := 0
endif

ifeq ($(RELEASE),0)
    override STRIP :=
endif
ifneq ($(FORCEDEBUG),0)
    override STRIP :=
endif

ifeq ($(RELEASE),0)
    OPTLEVEL := 0
    LTO := 0
else
    OPTLEVEL := 2
    LTO := 1
endif

ifneq (0,$(CLANG))
    ifeq ($(PLATFORM),WINDOWS)
        LTO := 0
    endif
endif
ifneq ($(LUNATIC),0)
    # FIXME: Lunatic builds with LTO don't start up properly as the required
    # symbol names are apparently not exported.
    override LTO := 0
endif
ifeq (0,$(CLANG))
    ifeq (0,$(GCC_PREREQ_4))
        override LTO := 0
    endif
    ifeq (4,$(GCC_MAJOR))
        ifeq ($(PLATFORM),WII)
            ifneq (,$(filter 0 1 2 3 4 5 6 7,$(GCC_MINOR)))
                override LTO := 0
            endif
        else
            ifneq (,$(filter 0 1 2 3 4 5,$(GCC_MINOR)))
                override LTO := 0
            endif
        endif
    endif
endif


########## End Toggles, Begin Construction ##########


##### Instantiate variables

COMMONFLAGS :=
COMPILERFLAGS := -funsigned-char

CSTD := -std=gnu99
CXXSTD := -std=gnu++11
ifneq (0,$(CLANG))
    CSTD := $(subst gnu,c,$(CSTD))
    CXXSTD := $(subst gnu,c,$(CXXSTD))
endif
CONLYFLAGS := $(CSTD)
CXXONLYFLAGS := $(CXXSTD) -fno-exceptions -fno-rtti

ASFLAGS := -s #-g

LUAJIT_BCOPTS :=

LINKERFLAGS :=
L_CXXONLYFLAGS :=

LIBS :=
GUI_LIBS :=
LIBDIRS :=


##### Mandatory platform parameters

ASFORMAT := elf$(BITS)
# Options to "luajit -b" for synthesis. Since it runs on Linux, we need to tell
# the native LuaJIT to emit PE object files.
ifeq ($(PLATFORM),WINDOWS)
    LINKERFLAGS += -static-libgcc -static
    ifeq (0,$(CLANG))
        L_CXXONLYFLAGS += -static-libstdc++
    endif

    ifeq (0,$(CLANG))
        GUI_LIBS += -mwindows
    endif

    COMPILERFLAGS += -DUNDERSCORES
    ASFORMAT := win$(BITS)
    ASFLAGS += -DUNDERSCORES

    ifneq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        LINKERFLAGS += -Wl,--large-address-aware
    endif
    LINKERFLAGS += -Wl,--enable-auto-import

    LUAJIT_BCOPTS := -o windows
    ifeq (32,$(BITS))
        LUAJIT_BCOPTS += -a x86
    endif
    ifeq (64,$(BITS))
        LUAJIT_BCOPTS += -a x64
    endif
else ifeq ($(PLATFORM),DARWIN)
    ifneq ($(ARCH),)
        ifneq ($(findstring -arch,$(ARCH)),-arch)
            override ARCH := -arch $(ARCH)
        endif
    endif
    COMMONFLAGS += $(ARCH)

    ifneq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        LINKERFLAGS += -read_only_relocs suppress
    endif

    COMPILERFLAGS += -DUNDERSCORES
    ASFORMAT := macho
    ASFLAGS += -DUNDERSCORES

    ifeq ($(findstring x86_64,$(IMPLICIT_ARCH)),x86_64)
        ASFORMAT += 64
    endif
else ifeq ($(PLATFORM),WII)
    LIBOGC_INC := $(DEVKITPRO)/libogc/include
    LIBOGC_LIB := $(DEVKITPRO)/libogc/lib/wii

    COMMONFLAGS += -mrvl -mcpu=750 -meabi -mhard-float
    LINKERFLAGS += -Wl,--gc-sections
    # -msdata=eabiexport
    COMPILERFLAGS += -DGEKKO -D__POWERPC__ -I$(LIBOGC_INC)
    LIBDIRS += -L$(LIBOGC_LIB)
else ifeq ($(PLATFORM),$(filter $(PLATFORM),DINGOO GCW))
    COMPILERFLAGS += -D__OPENDINGUX__
else ifeq ($(PLATFORM),SKYOS)
    COMPILERFLAGS += -DUNDERSCORES
endif
ASFLAGS += -f $(ASFORMAT)

COMPILERFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0


##### Optimizations

ifndef OPTOPT
    ifeq ($(findstring x86_64, $(IMPLICIT_ARCH)),x86_64)
        ifeq ($(findstring x86_64h, $(IMPLICIT_ARCH)),x86_64h)
            OPTOPT := -march=haswell -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt -mpclmul -mavx -mrdrnd -mf16c -mfsgsbase -mavx2 -maes -mfma -mbmi -mbmi2
            # -mcrc32 -mmovbe
        else
            ifeq ($(PLATFORM),DARWIN)
                OPTOPT := -march=core2 -mmmx -msse -msse2 -msse3 -mssse3
            endif
        endif
    endif
    ifeq ($(findstring i386, $(IMPLICIT_ARCH)),i386)
        ifeq ($(PLATFORM),DARWIN)
            OPTOPT := -march=nocona -mmmx -msse -msse2 -msse3
        else
            OPTOPT := -march=pentium3
            ifneq (0,$(GCC_PREREQ_4))
                OPTOPT += -mtune=generic
                # -mstackrealign
            endif
            OPTOPT += -mmmx
            # -msse2 -mfpmath=sse,387 -malign-double
        endif
    endif
    ifeq ($(PLATFORM),WII)
        OPTOPT := -mtune=750
    endif
endif

ifeq ($(PACKAGE_REPOSITORY),0)
    COMMONFLAGS += -O$(OPTLEVEL) $(OPTOPT)
endif

ifneq (0,$(LTO))
    COMPILERFLAGS += -DUSING_LTO
    COMMONFLAGS += -flto
endif


##### Debugging

ifneq ($(RELEASE)$(FORCEDEBUG),10)
    ifeq ($(PACKAGE_REPOSITORY),0)
        COMMONFLAGS += -g
        ifeq (0,$(CLANG))
            ifneq ($(PLATFORM),WII)
                COMMONFLAGS += -fno-omit-frame-pointer
            endif
        endif
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        # This option is needed to allow obtaining backtraces from within a program.
        LIBS += -rdynamic
    endif
endif

ifneq ($(ALLOCACHE_AS_MALLOC),0)
    COMPILERFLAGS += -DDEBUG_ALLOCACHE_AS_MALLOC
endif

# See https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
# and https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html
# for a list of possible ASan and UBsan options.

ASAN_FLAGS := -fsanitize=address -fsanitize=bounds,enum,float-cast-overflow
ASAN_FLAGS := $(ASAN_FLAGS),signed-integer-overflow,unsigned-integer-overflow
ASAN_FLAGS := $(ASAN_FLAGS),undefined,return,null,pointer-overflow,float-divide-by-zero
#ASAN_FLAGS := $(ASAN_FLAGS) -fsanitize-undefined-trap-on-error

ifeq (0,$(FORCEDEBUG))
    COMPILERFLAGS += -DNDEBUG
else
    COMPILERFLAGS += -DDEBUGGINGAIDS=$(FORCEDEBUG)

    ifeq (2,$(FORCEDEBUG))
        ifneq (0,$(CLANG))
            COMMONFLAGS += $(ASAN_FLAGS)
        else ifneq (,$(filter 1 2 3 4 5 6,$(GCC_MAJOR)))
            ifneq (,$(filter 0 1,$(GCC_MINOR)))
                COMMONFLAGS += $(ASAN_FLAGS)
            endif
        endif
    endif
endif

ifneq (0,$(KRANDDEBUG))
    COMPILERFLAGS += -DKRANDDEBUG=1
endif

ifneq (0,$(PROFILER))
    ifneq ($(PLATFORM),DARWIN)
        LIBS += -lprofiler
    endif
    COMMONFLAGS += -pg
endif


##### -f stuff

ifneq (0,$(GCC_PREREQ_4))
    F_NO_STACK_PROTECTOR := -fno-stack-protector
    ifeq (0,$(CLANG))
        F_JUMP_TABLES := -fjump-tables
    endif
endif

ifeq ($(PLATFORM),DARWIN)
    ifeq (1,$(DARWIN9))
        F_JUMP_TABLES :=
    endif
    ifeq ($(findstring ppc,$(IMPLICIT_ARCH))$(findstring i386,$(IMPLICIT_ARCH)),)
        F_NO_STACK_PROTECTOR :=
    endif
endif

ifeq (0,$(RELEASE))
    F_NO_STACK_PROTECTOR :=
else
    ifeq (0,$(CLANG))
        COMMONFLAGS += -funswitch-loops
    endif

    ifeq (0,$(FORCEDEBUG))
        COMMONFLAGS += -fomit-frame-pointer
    endif
endif

ifneq (0,$(KRANDDEBUG))
    COMMONFLAGS += -fno-inline -fno-inline-functions -fno-inline-functions-called-once
endif

COMMONFLAGS += -fno-strict-aliasing -fno-threadsafe-statics $(F_JUMP_TABLES) $(F_NO_STACK_PROTECTOR)


##### Warnings

W_STRICT_OVERFLOW := -Wno-strict-overflow

ifeq ($(PLATFORM),DARWIN)
    ifneq (0,$(DARWIN9))
        W_STRICT_OVERFLOW :=
    endif
endif

W_UNINITIALIZED := -Wuninitialized
W_GCC_4_1 := -Wno-attributes
W_GCC_4_2 := $(W_STRICT_OVERFLOW)
W_GCC_4_4 := -Wno-unused-result
W_GCC_4_5 := -Wlogical-op -Wcast-qual
W_CLANG := -Wno-unused-value -Wno-parentheses -Wno-unknown-warning-option

ifeq (0,$(CLANG))
    W_CLANG :=

    ifeq (0,$(GCC_PREREQ_4))
        W_GCC_4_5 :=
        W_GCC_4_4 :=
        ifeq (0,$(OPTLEVEL))
            W_UNINITIALIZED :=
        endif
        W_GCC_4_2 :=
        W_GCC_4_1 :=
    endif

    ifeq (4,$(GCC_MAJOR))
        ifneq (,$(filter 0 1 2 3 4,$(GCC_MINOR)))
            W_GCC_4_5 :=
            ifneq (,$(filter 0 1 2 3,$(GCC_MINOR)))
                W_GCC_4_4 :=
                ifeq (0,$(OPTLEVEL))
                    W_UNINITIALIZED :=
                endif
                ifneq (,$(filter 0 1,$(GCC_MINOR)))
                    W_GCC_4_2 :=
                    ifeq (0,$(GCC_MINOR))
                        W_GCC_4_1 :=
                    endif
                endif
            endif
        endif
    endif
endif

CONLYWARNS := -Wimplicit -Werror-implicit-function-declaration

CWARNS := -W -Wall \
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
    $(W_CLANG) \
    #-Wstrict-prototypes \
    #-Waggregate-return \
    #-Wcast-align \
    #-Waddress


##### Features

ifneq (,$(APPNAME))
    COMPILERFLAGS += -DAPPNAME=\"$(APPNAME)\"
endif
ifneq (,$(APPBASENAME))
    COMPILERFLAGS += -DAPPBASENAME=\"$(APPBASENAME)\"
endif

ifneq (0,$(NOASM))
    COMPILERFLAGS += -DNOASM
endif
ifneq (0,$(USE_ASM64))
    COMPILERFLAGS += -DUSE_ASM64
endif
ifneq (0,$(MEMMAP))
    ifeq ($(PLATFORM),DARWIN)
        LINKERFLAGS += -Wl,-map -Wl,$@.memmap
    else
        LINKERFLAGS += -Wl,-Map=$@.memmap
    endif
endif

COMPILERFLAGS += -DRENDERTYPE$(RENDERTYPE)=1 -DMIXERTYPE$(MIXERTYPE)=1

ifeq (0,$(NETCODE))
    COMPILERFLAGS += -DNETCODE_DISABLE
endif
ifneq (0,$(STARTUP_WINDOW))
    COMPILERFLAGS += -DSTARTUP_WINDOW
endif
ifneq (0,$(SIMPLE_MENU))
    COMPILERFLAGS += -DEDUKE32_SIMPLE_MENU
endif
ifneq (0,$(STANDALONE))
    COMPILERFLAGS += -DEDUKE32_STANDALONE
endif
ifneq (0,$(USE_OPENGL))
    COMPILERFLAGS += -DUSE_OPENGL
endif
ifneq (0,$(POLYMER))
    COMPILERFLAGS += -DPOLYMER
endif


##### External library paths

ifeq ($(PLATFORM),WINDOWS)
    COMPILERFLAGS += -Iplatform/Windows/include
    LIBDIRS += -Lplatform/Windows/lib/$(BITS)
else ifeq ($(PLATFORM),DARWIN)
    ifneq ($(shell port --version &>/dev/null; echo $$?),127)
        LIBDIRS += -L/opt/local/lib
        COMPILERFLAGS += -I/opt/local/include
    endif
    ifneq ($(shell brew --version &>/dev/null; echo $$?),127)
        LIBDIRS += -L/usr/local/lib
        COMPILERFLAGS += -I/usr/local/include
    endif
    ifneq ($(shell fink --version &>/dev/null; echo $$?),127)
        LIBDIRS += -L/sw/lib
        COMPILERFLAGS += -I/sw/include
    endif
else ifeq ($(PLATFORM),BSD)
    COMPILERFLAGS += -I/usr/local/include
else ifeq ($(PLATFORM),WII)
    COMPILERFLAGS += -I$(PORTLIBS)/include -Iplatform/Wii/include
    LIBDIRS += -L$(PORTLIBS)/lib -Lplatform/Wii/lib
endif


##### External libraries

ifneq ($(LUNATIC),0)
    ifneq ($(USE_LUAJIT_2_1),0)
        COMPILERFLAGS += -DUSE_LUAJIT_2_1
    endif

    ifeq ($(PLATFORM),WINDOWS)
        LIBS += -lluajit
    else
        LIBS += -lluajit-5.1
    endif
endif

ifneq (0,$(USE_LIBVPX))
    COMPILERFLAGS += -DUSE_LIBVPX
    LIBS += -lvpx
endif

ifneq (0,$(HAVE_VORBIS))
    COMPILERFLAGS += -DHAVE_VORBIS
endif
ifneq (0,$(HAVE_FLAC))
    COMPILERFLAGS += -DHAVE_FLAC
endif
ifneq (0,$(HAVE_XMP))
    COMPILERFLAGS += -DHAVE_XMP
endif

ifeq ($(RENDERTYPE),SDL)
    ifeq ($(SDL_TARGET),2)
        SDLCONFIG := sdl2-config
        SDLNAME := SDL2
    else ifeq ($(SDL_TARGET),1)
        SDLCONFIG := sdl-config
        SDLNAME := SDL
        ifeq (0,$(RELEASE))
            COMPILERFLAGS += -DNOSDLPARACHUTE
        endif
    endif

    ifeq ($(PLATFORM),WII)
        SDLCONFIG :=
    else ifeq ($(PLATFORM),SKYOS)
        COMPILERFLAGS += -I/boot/programs/sdk/include/sdl
        SDLCONFIG :=
    endif

    ifneq ($(strip $(SDLCONFIG)),)
        ifeq ($(strip $(shell $(SDLCONFIG) --version $(DONT_PRINT_STDERR))),)
            override SDLCONFIG :=
        endif
    endif

    COMPILERFLAGS += -DSDL_TARGET=$(SDL_TARGET)

    SDL_FRAMEWORK := 0
    ifneq ($(SDL_FRAMEWORK),0)
        ifeq ($(PLATFORM),DARWIN)
            APPLE_FRAMEWORKS := /Library/Frameworks
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

            COMPILERFLAGS += $(SDLCONFIG_CFLAGS)
            LIBS += $(SDLCONFIG_LIBS)
        else
            ifeq ($(SDL_TARGET),1)
                COMPILERFLAGS += -D_GNU_SOURCE=1
            endif
            COMPILERFLAGS += -D_REENTRANT -DSDL_USEFOLDER
            LIBS += -l$(SDLNAME)
        endif
    endif
endif

ifneq (0,$(HAVE_GTK2))
    ifneq (No,$(shell $(PKG_CONFIG) --exists gtk+-2.0 || echo No))
        COMPILERFLAGS += -DHAVE_GTK2 $(shell $(PKG_CONFIG) --cflags gtk+-2.0)
    else
        override HAVE_GTK2 := 0
    endif
endif


##### System libraries

ifeq ($(PLATFORM),WINDOWS)
    ifneq (0,$(GCC_PREREQ_4))
        L_SSP := -lssp
    endif
    LIBS += -lmingwex -lgdi32 -lpthread
    ifeq ($(RENDERTYPE),WIN)
        LIBS += -ldxguid
    else
        LIBS += -ldxguid_sdl -lmingw32 -limm32 -lole32 -loleaut32 -lversion
    endif
    LIBS += -lcomctl32 -lwinmm $(L_SSP) -lwsock32 -lws2_32 -lshlwapi
    # -lshfolder
else ifeq ($(PLATFORM),SKYOS)
    LIBS += -lnet
else ifeq ($(PLATFORM),QNX)
    LIBS += -lsocket
else ifeq ($(PLATFORM),SUNOS)
    LIBS += -lsocket -lnsl
else ifeq ($(PLATFORM),WII)
    LIBS += -laesnd_tueidj -lfat -lwiiuse -lbte -lwiikeyboard -logc
else ifeq ($(SUBPLATFORM),LINUX)
    LIBS += -lrt
endif

ifeq (,$(filter $(PLATFORM),WINDOWS WII))
    ifneq ($(PLATFORM),BSD)
        LIBS += -ldl
    endif
    ifneq ($(PLATFORM),DARWIN)
        LIBS += -pthread
    endif
endif

LIBS += -lm


##### Detect version control revision, if applicable

VC_REV :=
-include EDUKE32_REVISION.mak
ifeq (,$(VC_REV))
    VC_REV := $(word 2,$(subst :, ,$(filter Revision:%,$(subst : ,:,$(strip $(shell svn info 2>&1))))))
endif
ifeq (,$(VC_REV))
    GIT_SVN_URL := $(strip $(shell git config --local svn-remote.svn.url))
    GIT_SVN_FETCH := $(strip $(shell git config --local svn-remote.svn.fetch))
    VC_REV := $(word 2,$(subst @, ,$(filter git-svn-id:$(GIT_SVN_URL)@%,$(subst : ,:,$(shell git log -1 $(GIT_SVN_FETCH::%=%))))))
endif
ifneq (,$(VC_REV)$(VC_REV_CUSTOM))
    REVFLAG := -DREV="\"r$(VC_REV)$(VC_REV_CUSTOM)\""
endif


##### Allow standard environment variables to take precedence, to help package maintainers.

ifneq (,$(CFLAGS))
    COMMONFLAGS += $(CFLAGS)
endif
ifneq (,$(CXXFLAGS))
    CXXONLYFLAGS += $(CXXFLAGS)
endif
ifneq (,$(LDFLAGS))
    LINKERFLAGS += $(LDFLAGS)
endif


##### Final assembly of commands

COMPILER_C=$(CC) $(CONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(CONLYWARNS) $(COMPILERFLAGS) $(CUSTOMOPT)
COMPILER_CXX=$(CXX) $(CXXONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS) $(CUSTOMOPT)
COMPILER_OBJC=$(COBJC) $(CONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(CONLYWARNS) $(COMPILERFLAGS) $(CUSTOMOPT)
COMPILER_OBJCXX=$(COBJCXX) $(CXXONLYFLAGS) $(COMMONFLAGS) $(CWARNS) $(COMPILERFLAGS) $(CUSTOMOPT)
LINKER=$(L_CXX) $(CXXONLYFLAGS) $(L_CXXONLYFLAGS) $(COMMONFLAGS) $(LINKERFLAGS) $(CUSTOMOPT)
ifneq ($(CPLUSPLUS),0)
    COMPILER_C=$(COMPILER_CXX)
    COMPILER_OBJC=$(COMPILER_OBJCXX)
endif


##### Pretty-printing

ifeq ($(PRETTY_OUTPUT),1)
RECIPE_IF = if
BUILD_STARTED = printf "\033[K\033[1;36mBuilding: \033[0;36m$(MAKE) $(MAKECMDGOALS) -$(MAKEFLAGS)\033[0m\n"
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
