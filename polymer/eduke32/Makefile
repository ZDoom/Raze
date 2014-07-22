#
# EDuke32 Makefile for GNU Make
#

include Makefile.common


DUKE3D_SRC=source
DUKE3D_INC=$(DUKE3D_SRC)
DUKE3D_RSRC=rsrc
ENGINE_ROOT=build
ENGINE_SRC=$(ENGINE_ROOT)/src
ENGINE_INC=$(ENGINE_ROOT)/include
o=o
asm=nasm


# BUILD Engine

ENGINE_CFLAGS=-I$(ENGINE_SRC)
UTIL_LIBS= -lm # -lpthread

ifeq ($(PLATFORM),WINDOWS)
    ENGINE_OBJ=$(DUKE3D_SRC)/eobj_win
else
    ENGINE_OBJ=$(DUKE3D_SRC)/eobj
endif

ENGINE_OBJS=baselayer cache1d compat crc32 defs engine polymost texcache dxtfilter hightile textfont smalltextfont kplib lz4 osd pragmas scriptfile mutex xxhash
ENGINE_EDITOR_OBJS=build config defs
ifeq (0,$(NOASM))
  ENGINE_OBJS+= a
else
  ENGINE_OBJS+= a-c
  ifneq (0,$(USE_ASM64))
    ENGINE_OBJS+= a64
  endif
endif
ifeq (1,$(USE_OPENGL))
    ENGINE_OBJS+= mdsprite glbuild
    ifeq (1,$(POLYMER))
        ENGINE_OBJS+= polymer
    endif
endif
ifeq (1,$(LUNATIC))
    ENGINE_OBJS+= lunatic
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINE_OBJS += osxbits
    ifneq (0,$(OSX_STARTUPWINDOW))
        ENGINE_EDITOR_OBJS += startosx.editor
    endif
    ifeq (1,$(SDL_FRAMEWORK))
        ENGINE_OBJS+=SDLMain
    endif
endif
ifeq ($(PLATFORM),WINDOWS)
    ENGINE_OBJS+= winbits
    ENGINE_EDITOR_OBJS+= startwin.editor
endif
ifeq ($(RENDERTYPE),SDL)
    ENGINE_OBJS+= sdlayer

    ifeq (1,$(HAVE_GTK2))
        ENGINE_OBJS+= gtkbits
        ifeq ($(LINKED_GTK),0)
            ENGINE_OBJS+= dynamicgtk
        endif
        ENGINE_EDITOR_OBJS+= startgtk.editor
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    ENGINE_OBJS+= winlayer rawinput
endif

ENGINE_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(ENGINE_OBJS)))
ENGINE_EDITOR_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(ENGINE_EDITOR_OBJS)))


# AudioLib

AUDIOLIB_OBJS=drivers fx_man multivoc mix mixst pitch formats vorbis flac xa driver_nosound

AUDIOLIB_ROOT=$(DUKE3D_SRC)/jaudiolib
AUDIOLIB_SRC=$(AUDIOLIB_ROOT)/src
AUDIOLIB_INC=$(AUDIOLIB_ROOT)/include

AUDIOLIB_CFLAGS=-I$(AUDIOLIB_ROOT)/third-party/common/include -DHAVE_VORBIS
ifneq ($(PLATFORM),WII)
    AUDIOLIB_CFLAGS+= -DHAVE_FLAC
endif

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(MIXERTYPE),WIN)
        AUDIOLIB_CFLAGS+= -DHAVE_DS
        AUDIOLIB_OBJS+= driver_directsound
    endif

    AUDIOLIB_OBJ=$(AUDIOLIB_ROOT)/obj_win
else
    AUDIOLIB_OBJ=$(AUDIOLIB_ROOT)/obj
endif

ifeq ($(MIXERTYPE),SDL)
    AUDIOLIB_CFLAGS+= -DHAVE_SDL
    ifneq ($(PLATFORM),DARWIN)
        ifneq ($(PLATFORM),WINDOWS)
            ifneq ($(PLATFORM),WII)
                AUDIOLIB_CFLAGS+=`pkg-config --cflags vorbis`
            endif
        endif
    endif
    AUDIOLIB_OBJS+= driver_sdl
endif

AUDIOLIB_OBJS_EXP:=$(addprefix $(AUDIOLIB_OBJ)/,$(addsuffix .$o,$(AUDIOLIB_OBJS)))


# ENet

ENET_OBJS=callbacks host list packet peer protocol compress

ENET_ROOT=$(DUKE3D_SRC)/enet
ENET_SRC=$(ENET_ROOT)/src
ENET_INC=$(ENET_ROOT)/include
ENET_CFLAGS=

ifeq ($(PLATFORM),WINDOWS)
    ENET_OBJS += win32
    ENET_OBJ=$(ENET_ROOT)/obj_win
else
    ENET_OBJS += unix
    ENET_CFLAGS += -DHAS_SOCKLEN_T
    ENET_OBJ=$(ENET_ROOT)/obj
endif

ENET_OBJS_EXP:=$(addprefix $(ENET_OBJ)/,$(addsuffix .$o,$(ENET_OBJS)))


# Tools

UTIL_OBJS=compat pragmas kplib cache1d compat_tools
UTILS=kextract kgroup transpal wad2art wad2map kmd2tool md2tool generateicon cacheinfo arttool givedepth mkpalette unpackssi bsuite
GAMEUTILS=ivfrate
DXUTILS=enumdisplay getdxdidf
SDLUTILS=makesdlkeytrans

UTIL_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(UTIL_OBJS)))


# Duke Nukem 3D

ifneq (0,$(RELEASE))
    # Debugging disabled
    COMMONFLAGS += $(F_NO_STACK_PROTECTOR)
else
    # Debugging enabled
    ifneq (0,$(KRANDDEBUG))
        COMMONFLAGS += -fno-inline -fno-inline-functions -fno-inline-functions-called-once
    endif
    ifeq (1,$(SDL_TARGET))
        COMPILERFLAGS += -DNOSDLPARACHUTE
    endif
endif

COMPILERFLAGS += -I$(DUKE3D_INC) -I$(ENGINE_INC) -I$(DUKE3D_SRC)/jmact -I$(AUDIOLIB_ROOT)/include -I$(ENET_ROOT)/include

# Game/editor-specific linker options
DUKE3D_LINKERFLAGS=
DUKE3D_EDITOR_LINKERFLAGS=


ifeq ($(NETCODE),0)
    ENET_TARGET=
else
    ENET_TARGET=$(ENET_OBJS_EXP)
endif

EDUKE32 ?= eduke32$(EXESUFFIX)
MAPSTER32 ?= mapster32$(EXESUFFIX)

EDUKE32_TARGET:=$(EDUKE32)
MAPSTER32_TARGET:=$(MAPSTER32)

ifndef EBACKTRACEDLL
    EBACKTRACEDLL = ebacktrace1.dll
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        EBACKTRACEDLL = ebacktrace1-64.dll
    endif
endif
EBACKTRACEDLL_TARGET:=$(EBACKTRACEDLL)

ifeq ($(PLATFORM),WINDOWS)
    DUKE3D_OBJ=$(DUKE3D_SRC)/obj_win
else
    ifeq ($(SUBPLATFORM),LINUX)
        LIBS+= -lrt
    endif
    DUKE3D_OBJ=$(DUKE3D_SRC)/obj
endif

MACT_OBJ=file_lib control keyboard mouse joystick mathutil scriplib animlib

DUKE3D_OBJS=game actors anim common rev config demo gamedef gameexec gamevars global input menus namesdyn net player premap savegame sector rts osdfuncs osdcmds grpscan sounds soundsdyn $(MACT_OBJ)

DUKE3D_EDITOR_OBJS=astub common rev m32def m32exec m32vars mathutil sounds_mapster32

ifneq ($(USE_LIBVPX),0)
    DUKE3D_OBJS+= animvpx
endif

ifneq (0,$(DISABLEINLINING))
    DUKE3D_OBJS+= game_inline actors_inline sector_inline
endif

DUKE3D_MISCDEPS=
DUKE3D_EDITOR_MISCDEPS=


## Lunatic devel
ifneq (0,$(LUNATIC))
    LUNATIC_COMMON_OBJS = luaJIT_BC_defs_common luaJIT_BC_engine_maptext luaJIT_BC_engine luaJIT_BC_bcarray luaJIT_BC_bcheck luaJIT_BC_bitar luaJIT_BC_xmath luaJIT_BC_v luaJIT_BC_dump luaJIT_BC_dis_x86 luaJIT_BC_dis_x64

    # TODO: remove debugging modules from release build

    DUKE3D_EDITOR_OBJS+= lunatic_m32 $(LUNATIC_COMMON_OBJS)
    DUKE3D_OBJS+= lunatic_game $(LUNATIC_COMMON_OBJS)

    DUKE3D_EDITOR_OBJS+= luaJIT_BC_defs_m32

    ifneq ($(PLATFORM),WINDOWS)
        # On non-Windows, we expect to have liblpeg.a (or a symlink to it) in source/.
        # On Windows, it will reside in platform/Windows/lib/32/ or lib/64/.
        LIBDIRS+= -L$(DUKE3D_SRC)
        ifeq ($(realpath $(DUKE3D_SRC)/liblpeg.a),)
            # XXX: This cripples "make clean" etc. too, but IMO it's better than warning.
            $(error "liblpeg.a not found in $(realpath $(DUKE3D_OBJ)/..)")
        endif
    endif
    LIBS+= -llpeg
    DUKE3D_OBJS+= luaJIT_BC_con_lang luaJIT_BC_lunacon luaJIT_BC_randgen luaJIT_BC_stat luaJIT_BC_control luaJIT_BC_defs luaJIT_BC_savegame luaJIT_BC_fs

    # now, take care of having the necessary symbols (sector, wall, etc.) in the
    # executable no matter what the debugging level

    ifeq ($(PLATFORM),DARWIN)
        # strip on OSX says: removing global symbols from a final linked no longer supported.
        #                    Use -exported_symbols_list at link time when building
        # But, following _their_ directions does not give us the symbols! wtf?
        ifneq ($(STRIP),0)
            STRIP+= -s $(DUKE3D_SRC)/lunatic/dynsymlist_osx
        endif

        DUKE3D_MISCDEPS+= $(DUKE3D_SRC)/lunatic/dynsymlist_osx
        LINKERFLAGS+= -pagezero_size 10000 -image_base 100000000 #-Wl,-alias_list -Wl,$(DUKE3D_SRC)/lunatic/aliases_list #-exported_symbols_list $(DUKE3D_SRC)/lunatic/dynsymlist_osx
    endif
    ifeq ($(PLATFORM),WINDOWS)
        override STRIP=
        DUKE3D_MISCDEPS+= $(DUKE3D_SRC)/lunatic/eduke32.def
        DUKE3D_LINKERFLAGS+= $(DUKE3D_SRC)/lunatic/eduke32.def
        DUKE3D_EDITOR_MISCDEPS+= $(DUKE3D_SRC)/lunatic/mapster32.def
        DUKE3D_EDITOR_LINKERFLAGS+= $(DUKE3D_SRC)/lunatic/mapster32.def
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        override STRIP=
        DUKE3D_LINKERFLAGS+= -Wl,--dynamic-list=$(DUKE3D_SRC)/lunatic/dynsymlist
        DUKE3D_EDITOR_LINKERFLAGS+= -Wl,--dynamic-list=$(DUKE3D_SRC)/lunatic/dynsymlist_m32
    endif
endif


# PLATFORM SPECIFIC SETTINGS

ifeq ($(SUBPLATFORM),LINUX)
    ifeq (0,$(CLANG))
        COMMONFLAGS	+= -fno-pic
    endif
    LIBS		+= -lFLAC -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),DARWIN)
    COMMONFLAGS += -fno-pic
    LIBDIRS += -L$(AUDIOLIB_ROOT)/third-party/Apple/lib

    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm \
            -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
            -Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
            -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
            -Wl,-framework,QuickTime -lm

    ifneq (0,$(OSX_STARTUPWINDOW))
        DUKE3D_OBJS += GrpFile.game GameListSource.game startosx.game
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    COMMONFLAGS += -fno-pic
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lcompat-from-mingw-w64
    LIBDIRS += -L$(AUDIOLIB_ROOT)/third-party/Windows/lib$(WINLIB)
    DUKE3D_OBJS+= gameres winbits startwin.game
    DUKE3D_EDITOR_OBJS+= buildres
    ifeq ($(MIXERTYPE),WIN)
        LIBS+= -ldsound
        DUKE3D_OBJS+= music midi mpu401
    endif
endif

ifeq ($(PLATFORM),WII)
    LIBS += -lvorbisidec
endif

ifeq ($(RENDERTYPE),SDL)
    ifeq (1,$(HAVE_GTK2))
        DUKE3D_OBJS+= game_banner startgtk.game
        DUKE3D_EDITOR_OBJS+= editor_banner
    endif

    DUKE3D_OBJS+= game_icon
    DUKE3D_EDITOR_OBJS+= build_icon
endif
ifeq ($(MIXERTYPE),SDL)
    DUKE3D_OBJS+= sdlmusic
endif


DUKE3D_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_OBJS)))
DUKE3D_EDITOR_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_EDITOR_OBJS)))




ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: veryclean clean all cleanutils utils dxutils sdlutils printutils printsdlutils printdxutils rev rev_clean

# TARGETS

all: start $(DO_REV) $(EDUKE32_TARGET) $(MAPSTER32_TARGET) finish
ifneq (,$(EDUKE32_TARGET))
	@ls -l $(EDUKE32)
endif
ifneq (,$(MAPSTER32_TARGET))
	@ls -l $(MAPSTER32)
endif

ebacktrace: start $(EBACKTRACEDLL_TARGET) finish
ifneq (,$(EBACKTRACEDLL_TARGET))
	@ls -l $(EBACKTRACEDLL)
endif

start:
	$(BUILD_STARTED)

finish:
	$(BUILD_FINISHED)

utils: $(addsuffix $(EXESUFFIX),$(UTILS) $(GAMEUTILS))
	@ls -l $^

dxutils: $(addsuffix $(EXESUFFIX),$(DXUTILS))
	@ls -l $^

sdlutils: $(addsuffix $(EXESUFFIX),$(SDLUTILS))
	@ls -l $^

$(EDUKE32): $(DUKE3D_OBJS_EXP) $(ENGINE_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(ENET_TARGET) $(DUKE3D_MISCDEPS)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifneq ($(STRIP),)
	$(STRIP) $(EDUKE32)
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/EDuke32.app" "./"
	mkdir -p "EDuke32.app/Contents/MacOS"
	cp -f "$(EDUKE32)" "EDuke32.app/Contents/MacOS/"
endif

$(MAPSTER32): $(DUKE3D_EDITOR_OBJS_EXP) $(ENGINE_EDITOR_OBJS_EXP) $(ENGINE_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(DUKE3D_EDITOR_MISCDEPS)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_EDITOR_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifneq ($(STRIP),)
	$(STRIP) $(MAPSTER32)
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/Mapster32.app" "./"
	mkdir -p "Mapster32.app/Contents/MacOS"
	cp -f "$(MAPSTER32)" "Mapster32.app/Contents/MacOS/"
endif

include Makefile.deps
include $(ENGINE_ROOT)/Makefile.deps

# RULES

$(EBACKTRACEDLL): platform/Windows/src/backtrace.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CC) $(CONLYFLAGS) -O2 -ggdb -shared -Wall -Wextra -static-libgcc -I$(ENGINE_INC) -o $@ $^ -lbfd -liberty -limagehlp $(RECIPE_RESULT_COMPILE)

libcache1d$(DLLSUFFIX): $(ENGINE_SRC)/cache1d.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) -Wall -Wextra -DCACHE1D_COMPRESS_ONLY -shared -fPIC $< -o $@ $(RECIPE_RESULT_COMPILE)

%$(EXESUFFIX): $(ENGINE_OBJ)/%.$o $(UTIL_OBJS_EXP)
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(UTIL_LIBS) $(RECIPE_RESULT_ONESTEP)

%$(EXESUFFIX): $(DUKE3D_OBJ)/%.$o
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(UTIL_LIBS) $(RECIPE_RESULT_ONESTEP)

enumdisplay$(EXESUFFIX): $(ENGINE_OBJ)/enumdisplay.$o
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(UTIL_LIBS) -lgdi32 $(RECIPE_RESULT_ONESTEP)
getdxdidf$(EXESUFFIX): $(ENGINE_OBJ)/getdxdidf.$o
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(UTIL_LIBS) -ldinput $(RECIPE_RESULT_ONESTEP)
makesdlkeytrans$(EXESUFFIX): $(ENGINE_OBJ)/makesdlkeytrans.$o
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(UTIL_LIBS) $(SDL_INCLUDES) $(RECIPE_RESULT_ONESTEP)
arttool$(EXESUFFIX): $(ENGINE_OBJ)/arttool.$o
	$(ONESTEP_STATUS)
	$(RECIPE_IF) $(L_CXX) $(CPPONLYFLAGS) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(STDCPPLIB) $(UTIL_LIBS) $(RECIPE_RESULT_ONESTEP)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

#### Lunatic

# Create object files directly with luajit
$(DUKE3D_OBJ)/luaJIT_BC_%.$o: source/lunatic/%.lua
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(LUAJIT) -bg $(LUAJIT_BCOPTS) $< $@ $(RECIPE_RESULT_COMPILE)

# Same thing for defs*.ilua which I'm too reluctant to rename now:
# NOTE: The target path must match EXACTLY with that of the DEFS_BC_SIZE
# determination in Makefile.common, because it is embedded into the bytecode as
# debugging information.
$(DUKE3D_OBJ)/luaJIT_BC_%.$o: source/lunatic/%.ilua
	$(RECIPE_IF) $(LUAJIT) -bg $(LUAJIT_BCOPTS) $< $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/lunatic/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

# TODO: _m32
# List of exported symbols, OS X
$(DUKE3D_SRC)/lunatic/dynsymlist_osx: $(DUKE3D_SRC)/lunatic/dynsymlist
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

#$(DUKE3D_SRC)/lunatic/aliases_list: $(DUKE3D_SRC)/lunatic/dynsymlist_osx
#	sed 's/_\([A-Za-z_][A-Za-z_0-9]*\)/_\1 \1/g' $< > $@

# List of exported symbols, Windows
$(DUKE3D_SRC)/lunatic/eduke32.def: $(DUKE3D_SRC)/lunatic/dynsymlist
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@

$(DUKE3D_SRC)/lunatic/mapster32.def: $(DUKE3D_SRC)/lunatic/dynsymlist_m32
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@

####

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.nasm
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(AS) $(ASFLAGS) $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.yasm
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(AS) $(ASFLAGS) $< -o $@ $(RECIPE_RESULT_COMPILE)

# Comment out the following rule to debug a-c.o
$(ENGINE_OBJ)/a-c.$o: $(ENGINE_SRC)/a-c.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(subst -O$(OPTLEVEL),-O2,$(subst $(CLANG_DEBUG_FLAGS),,$(COMMONFLAGS) $(COMPILERFLAGS))) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/util/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.m
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.cpp
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/misc/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/misc/%.rc
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(RC) -i $< -o $@ --include-dir=$(ENGINE_INC) --include-dir=$(ENGINE_SRC) $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.cpp
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.cc
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(DUKE3D_RSRC)/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(AUDIOLIB_OBJ)/%.o: $(AUDIOLIB_SRC)/%.c | $(AUDIOLIB_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(AUDIOLIB_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(AUDIOLIB_OBJ):
	mkdir $(AUDIOLIB_OBJ)

$(ENET_OBJ)/%.o: $(ENET_SRC)/%.c $(ENET_INC)/enet/*.h | $(ENET_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENET_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENET_OBJ):
	mkdir $(ENET_OBJ)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.m
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.cpp
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/jmact/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/misc/%.rc
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(RC) -i $< -o $@ --include-dir=$(ENGINE_INC) --include-dir=$(DUKE3D_SRC) -DPOLYMER=$(POLYMER) $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_RSRC)/%.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -Wno-pointer-sign -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_RSRC)/game_banner.c: $(DUKE3D_RSRC)/game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(DUKE3D_RSRC)/editor_banner.c: $(DUKE3D_RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES

clean: $(UNDO_REV)
	-rm -f $(EDUKE32) $(MAPSTER32) $(DUKE3D_OBJS_EXP) $(DUKE3D_EDITOR_OBJS_EXP) $(DUKE3D_MISCDEPS) $(DUKE3D_EDITOR_MISCDEPS) core*
ifeq ($(PLATFORM),DARWIN)
	-rm -rf EDuke32.app Mapster32.app
endif

cleanutils:
	-rm -f $(addsuffix $(EXESUFFIX),$(UTILS) $(GAMEUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(UTILS))) $(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(GAMEUTILS))) $(addsuffix $(EXESUFFIX),$(DXUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(DXUTILS))) $(addsuffix $(EXESUFFIX),$(SDLUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(SDLUTILS))) $(UTIL_OBJS_EXP)

veryclean: clean cleanutils
	-rm -f $(ENGINE_EDITOR_OBJS_EXP) $(ENGINE_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(ENET_OBJS_EXP) $(DUKE3D_RSRC)/*banner* $(EBACKTRACEDLL)

printutils:
	echo "$(addsuffix $(EXESUFFIX),$(UTILS) $(GAMEUTILS))"

printdxutils:
	echo "$(addsuffix $(EXESUFFIX),$(DXUTILS))"

printsdlutils:
	echo "$(addsuffix $(EXESUFFIX),$(SDLUTILS))"

rev:
	@echo "s_buildRev = \"r$(VC_REV)$(VC_REV_CUSTOM)\";">source/rev.h

rev_clean:
	@$(VC_CLEAN) source/rev.h
