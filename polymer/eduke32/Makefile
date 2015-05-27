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
obj=obj

COMPILERFLAGS += -I$(ENGINE_INC) -I$(MACT_INC) -I$(AUDIOLIB_INC) -I$(ENET_INC)


# EBacktrace

ifndef EBACKTRACEDLL
    EBACKTRACEDLL = ebacktrace1.dll
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        EBACKTRACEDLL = ebacktrace1-64.dll
    endif
endif
EBACKTRACEDLL_TARGET:=$(EBACKTRACEDLL)


# BUILD Engine

ENGINE_CFLAGS=-I$(ENGINE_SRC)
UTIL_LIBS= -lm # -lpthread

ENGINE_OBJ=$(ENGINE_ROOT)/$(obj)

ENGINE_OBJS=baselayer cache1d common compat crc32 defs engine polymost texcache dxtfilter hightile textfont smalltextfont kplib lz4 osd pragmas scriptfile mmulti_null mutex xxhash md4
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
    ENGINE_OBJS+= glbuild voxmodel mdsprite
    ifeq (1,$(POLYMER))
        ENGINE_OBJS+= polymer
    endif
endif
ifneq (0,$(LUNATIC))
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
ifeq ($(PLATFORM),WII)
    ENGINE_OBJS+= wiibits
    LINKERFLAGS+= -Wl,-wrap,c_default_exceptionhandler
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


# MACT

MACT_ROOT=$(DUKE3D_SRC)/jmact
MACT_SRC=$(MACT_ROOT)
MACT_INC=$(MACT_ROOT)
MACT_OBJ=$(MACT_ROOT)/$(obj)

MACT_OBJS=file_lib control keyboard mouse joystick scriplib animlib

MACT_OBJS_EXP:=$(addprefix $(MACT_OBJ)/,$(addsuffix .$o,$(MACT_OBJS)))


# AudioLib

AUDIOLIB_OBJS=drivers fx_man multivoc mix mixst pitch formats vorbis flac xa driver_nosound

AUDIOLIB_ROOT=$(DUKE3D_SRC)/jaudiolib
AUDIOLIB_SRC=$(AUDIOLIB_ROOT)/src
AUDIOLIB_INC=$(AUDIOLIB_ROOT)/include
AUDIOLIB_OBJ=$(AUDIOLIB_ROOT)/$(obj)

AUDIOLIB_CFLAGS=-I$(AUDIOLIB_ROOT)/third-party/common/include

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(MIXERTYPE),WIN)
        AUDIOLIB_CFLAGS+= -DHAVE_DS
        AUDIOLIB_OBJS+= driver_directsound
    endif
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
ENET_OBJ=$(ENET_ROOT)/$(obj)

ENET_CFLAGS=

ifeq ($(PLATFORM),WINDOWS)
    ENET_OBJS += win32
else
    ENET_OBJS += unix
    ENET_CFLAGS += -DHAS_SOCKLEN_T
endif

ENET_OBJS_EXP:=$(addprefix $(ENET_OBJ)/,$(addsuffix .$o,$(ENET_OBJS)))

ifeq ($(NETCODE),0)
    ENET_TARGET=
else
    ENET_TARGET=$(ENET_OBJS_EXP)
endif


# Tools

UTIL_OBJS=compat pragmas kplib cache1d crc32 compat_tools
UTILS=kextract kgroup transpal wad2art wad2map kmd2tool md2tool generateicon cacheinfo arttool givedepth mkpalette unpackssi bsuite
GAMEUTILS=ivfrate
DXUTILS=enumdisplay getdxdidf
SDLUTILS=makesdlkeytrans

UTIL_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(UTIL_OBJS)))


# KenBuild (Test Game)

TESTGAME_ROOT=$(DUKE3D_SRC)/testgame
TESTGAME_SRC=$(TESTGAME_ROOT)/src
TESTGAME_RSRC=$(TESTGAME_ROOT)/rsrc
TESTGAME_OBJ=$(TESTGAME_ROOT)/$(obj)

TEST_CFLAGS=-I$(TESTGAME_SRC)

TESTGAME ?= testgame
TESTEDITOR ?= testeditor

TESTGAME_TARGET:=$(TESTGAME)$(EXESUFFIX)
TESTEDITOR_TARGET:=$(TESTEDITOR)$(EXESUFFIX)

TESTGAME_OBJS=game sound_stub common config
TESTEDITOR_OBJS=bstub common

ifeq ($(RENDERTYPE),SDL)
	ifeq (1,$(HAVE_GTK2))
		TESTGAME_OBJS+= game_banner startgtk.game
		TESTEDITOR_OBJS+= build_banner
	endif

	TESTGAME_OBJS+= game_icon
	TESTEDITOR_OBJS+= build_icon
endif
ifeq ($(PLATFORM),WINDOWS)
	TESTGAME_OBJS+= gameres startwin.game
	TESTEDITOR_OBJS+= buildres
endif

TESTGAME_OBJS_EXP:=$(addprefix $(TESTGAME_OBJ)/,$(addsuffix .$o,$(TESTGAME_OBJS)))
TESTEDITOR_OBJS_EXP:=$(addprefix $(TESTGAME_OBJ)/,$(addsuffix .$o,$(TESTEDITOR_OBJS)))


# Duke Nukem 3D

DUKE3D_CFLAGS=-I$(DUKE3D_INC)

DUKE3D_LINKERFLAGS=
DUKE3D_EDITOR_LINKERFLAGS=

DUKE3D_OBJ=$(DUKE3D_SRC)/$(obj)

EDUKE32 ?= eduke32
MAPSTER32 ?= mapster32

EDUKE32_TARGET:=$(EDUKE32)$(EXESUFFIX)
MAPSTER32_TARGET:=$(MAPSTER32)$(EXESUFFIX)

COMMON_OBJS=rev
COMMON_EDITOR_OBJS=m32common m32def m32exec m32vars rev

DUKE3D_OBJS=game global actors gamedef gameexec gamevars player premap sector anim animsounds common config demo input menus namesdyn net savegame rts osdfuncs osdcmds grpscan sounds soundsdyn
DUKE3D_EDITOR_OBJS=astub common grpscan sounds_mapster32

ifneq ($(USE_LIBVPX),0)
    DUKE3D_OBJS+= animvpx
endif

ifneq (0,$(DISABLEINLINING))
    DUKE3D_OBJS+= game_inline actors_inline sector_inline
endif

DUKE3D_MISCDEPS=
DUKE3D_EDITOR_MISCDEPS=

# Lunatic object base names. These are not used in targets directly.
LUNATIC_COMMON_OBJS = luaJIT_BC_defs_common luaJIT_BC_engine_maptext luaJIT_BC_engine luaJIT_BC_bcarray luaJIT_BC_bcheck luaJIT_BC_bitar luaJIT_BC_xmath luaJIT_BC_v luaJIT_BC_dump luaJIT_BC_dis_x86 luaJIT_BC_dis_x64
LUNATIC_GAME_OBJS = luaJIT_BC_con_lang luaJIT_BC_lunacon luaJIT_BC_randgen luaJIT_BC_stat luaJIT_BC_control luaJIT_BC_defs luaJIT_BC_savegame luaJIT_BC_fs

## Lunatic devel
ifneq (0,$(LUNATIC))
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
    DUKE3D_OBJS+= $(LUNATIC_GAME_OBJS)

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

ifeq ($(SUBPLATFORM),LINUX)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),BSD)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lexecinfo
endif

ifeq ($(PLATFORM),DARWIN)
    # LIBDIRS += -L$(AUDIOLIB_ROOT)/third-party/Apple/lib

    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm \
            -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
            -Wl,-framework,CoreMIDI -Wl,-framework,AudioUnit \
            -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL
    ifneq (00,$(DARWIN9)$(DARWIN10))
        LIBS += -Wl,-framework,QuickTime -lm
    endif

    ifneq (0,$(OSX_STARTUPWINDOW))
        DUKE3D_OBJS += GrpFile.game GameListSource.game startosx.game
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lcompat-from-mingw-w64
    LIBDIRS += -L$(AUDIOLIB_ROOT)/third-party/Windows/lib$(WINLIB)
    DUKE3D_OBJS+= gameres winbits startwin.game
    DUKE3D_EDITOR_OBJS+= buildres
    ifeq ($(MIXERTYPE),WIN)
        LIBS+= -ldsound
        MIDI_OBJS=music midi mpu401
    endif
endif

ifeq ($(PLATFORM),WII)
    LIBS += -lvorbisidec
endif

ifeq ($(RENDERTYPE),SDL)
    ifeq (1,$(HAVE_GTK2))
        DUKE3D_OBJS+= game_banner startgtk.game
        DUKE3D_EDITOR_OBJS+= build_banner
    endif

    DUKE3D_OBJS+= game_icon
    DUKE3D_EDITOR_OBJS+= build_icon
endif
ifeq ($(MIXERTYPE),SDL)
    MIDI_OBJS=sdlmusic
endif

## Construct file names of object files

COMMON_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(COMMON_OBJS)))
COMMON_EDITOR_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(COMMON_EDITOR_OBJS)))

MIDI_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(MIDI_OBJS)))

DUKE3D_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_OBJS)))
DUKE3D_EDITOR_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_EDITOR_OBJS)))


# Shadow Warrior

SW_ROOT=$(DUKE3D_SRC)/sw
SW_SRC=$(SW_ROOT)/src
SW_INC=$(SW_SRC)
SW_RSRC=$(SW_ROOT)/rsrc
SW_OBJ=$(SW_ROOT)/$(obj)

SW_CFLAGS=-I$(SW_INC)

SW ?= voidsw
SW_EDITOR ?= voidsw-editor

SW_TARGET:=$(SW)$(EXESUFFIX)
SW_EDITOR_TARGET:=$(SW_EDITOR)$(EXESUFFIX)

SW_OBJS=actor ai anim border break bunny cache cheats colormap common config console coolg coolie copysect demo draw eel game girlninj goro grpscan hornet interp interpsh inv jplayer jsector jweapon lava light mclip mdastr menus miscactr morph net ninja panel player predict quake ripper ripper2 rooms rotator rts save scrip2 sector serp setup skel skull slidor sounds spike sprite sumo swconfig sync text track vator vis wallmove warp weapon zilla zombie saveable
SW_EDITOR_OBJS=jnstub brooms bldscript jbhlp colormap grpscan common

ifeq ($(RENDERTYPE),SDL)
    ifeq (1,$(HAVE_GTK2))
        SW_OBJS+= game_banner startgtk.game
        SW_EDITOR_OBJS+= build_banner
    endif

    SW_OBJS+= game_icon
    SW_EDITOR_OBJS+= game_icon
endif
ifeq ($(PLATFORM),WINDOWS)
	SW_OBJS+= gameres startwin.game
	SW_EDITOR_OBJS+= buildres
endif

SW_OBJS_EXP:=$(addprefix $(SW_OBJ)/,$(addsuffix .$o,$(SW_OBJS)))
SW_EDITOR_OBJS_EXP:=$(addprefix $(SW_OBJ)/,$(addsuffix .$o,$(SW_EDITOR_OBJS)))


ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: all duke3d test sw veryclean clean cleanduke3d cleantest cleansw cleanutils utils dxutils sdlutils printutils printsdlutils printdxutils rev $(DUKE3D_OBJ)/rev.$o

# TARGETS

all: duke3d

duke3d: start $(EDUKE32_TARGET) $(MAPSTER32_TARGET) finish
ifneq (,$(EDUKE32_TARGET))
	@ls -l $(EDUKE32)$(EXESUFFIX)
endif
ifneq (,$(MAPSTER32_TARGET))
	@ls -l $(MAPSTER32)$(EXESUFFIX)
endif

test: start $(TESTGAME_TARGET) $(TESTEDITOR_TARGET) finish
ifneq (,$(TESTGAME_TARGET))
	@ls -l $(TESTGAME)$(EXESUFFIX)
endif
ifneq (,$(TESTEDITOR_TARGET))
	@ls -l $(TESTEDITOR)$(EXESUFFIX)
endif

sw: start $(SW_TARGET) $(SW_EDITOR_TARGET) finish
ifneq (,$(SW_TARGET))
	@ls -l $(SW)$(EXESUFFIX)
endif
ifneq (,$(SW_EDITOR_TARGET))
	@ls -l $(SW_EDITOR)$(EXESUFFIX)
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

ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
$(TESTGAME)$(DOLSUFFIX): $(TESTGAME)$(EXESUFFIX)
$(TESTEDITOR)$(DOLSUFFIX): $(TESTEDITOR)$(EXESUFFIX)
$(EDUKE32)$(DOLSUFFIX): $(EDUKE32)$(EXESUFFIX)
$(MAPSTER32)$(DOLSUFFIX): $(MAPSTER32)$(EXESUFFIX)
$(SW)$(DOLSUFFIX): $(SW)$(EXESUFFIX)
$(SW_EDITOR)$(DOLSUFFIX): $(SW_EDITOR)$(EXESUFFIX)
endif
endif

$(TESTGAME)$(EXESUFFIX): $(TESTGAME_OBJS_EXP) $(COMMON_OBJS_EXP) $(ENGINE_OBJS_EXP)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(TESTGAME)$(EXESUFFIX) $(TESTGAME)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif

$(TESTEDITOR)$(EXESUFFIX): $(TESTEDITOR_OBJS_EXP) $(COMMON_EDITOR_OBJS_EXP) $(ENGINE_EDITOR_OBJS_EXP) $(ENGINE_OBJS_EXP)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(TESTEDITOR)$(EXESUFFIX) $(TESTEDITOR)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif

$(EDUKE32)$(EXESUFFIX): $(DUKE3D_OBJS_EXP) $(COMMON_OBJS_EXP) $(MIDI_OBJS_EXP) $(ENGINE_OBJS_EXP) $(MACT_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(ENET_TARGET) $(DUKE3D_MISCDEPS)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(EDUKE32)$(EXESUFFIX) $(EDUKE32)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/EDuke32.app" "./"
	mkdir -p "EDuke32.app/Contents/MacOS"
	cp -f "$(EDUKE32)$(EXESUFFIX)" "EDuke32.app/Contents/MacOS/"
endif

$(MAPSTER32)$(EXESUFFIX): $(DUKE3D_EDITOR_OBJS_EXP) $(COMMON_EDITOR_OBJS_EXP) $(ENGINE_EDITOR_OBJS_EXP) $(ENGINE_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(DUKE3D_EDITOR_MISCDEPS)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_EDITOR_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(MAPSTER32)$(EXESUFFIX) $(MAPSTER32)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/Mapster32.app" "./"
	mkdir -p "Mapster32.app/Contents/MacOS"
	cp -f "$(MAPSTER32)$(EXESUFFIX)" "Mapster32.app/Contents/MacOS/"
endif

$(SW)$(EXESUFFIX): $(SW_OBJS_EXP) $(COMMON_OBJS_EXP) $(MIDI_OBJS_EXP) $(ENGINE_OBJS_EXP) $(MACT_OBJS_EXP) $(AUDIOLIB_OBJS_EXP)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(SW)$(EXESUFFIX) $(SW)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif

$(SW_EDITOR)$(EXESUFFIX): $(SW_EDITOR_OBJS_EXP) $(COMMON_EDITOR_OBJS_EXP) $(ENGINE_EDITOR_OBJS_EXP) $(ENGINE_OBJS_EXP)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(COMMONFLAGS) $(LINKERFLAGS) $(DUKE3D_EDITOR_LINKERFLAGS) $(LIBDIRS) $(LIBS) $(STATICSTDCPP) $(RECIPE_RESULT_LINK)
ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
	$(ELF2DOL) $(SW_EDITOR)$(EXESUFFIX) $(SW_EDITOR)$(DOLSUFFIX)
endif
endif
ifneq ($(STRIP),)
	$(STRIP) $@
endif

include $(ENGINE_ROOT)/Makefile.deps
include Makefile.deps
include $(SW_ROOT)/Makefile.deps

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
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

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

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.nasm | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(AS) $(ASFLAGS) $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.yasm | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(AS) $(ASFLAGS) $< -o $@ $(RECIPE_RESULT_COMPILE)

# Comment out the following rule to debug a-c.o
$(ENGINE_OBJ)/a-c.$o: $(ENGINE_SRC)/a-c.c | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(subst -O$(OPTLEVEL),-O2,$(subst $(CLANG_DEBUG_FLAGS),,$(COMMONFLAGS) $(COMPILERFLAGS))) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.c | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.m | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_OBJC) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/%.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/misc/%.c | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.c | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(ENGINE_SRC)/util/%.cc | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/%.$o: $(DUKE3D_RSRC)/%.c | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(MACT_OBJ)/%.$o: $(MACT_SRC)/%.c | $(MACT_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(AUDIOLIB_OBJ)/%.o: $(AUDIOLIB_SRC)/%.c | $(AUDIOLIB_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(AUDIOLIB_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENET_OBJ)/%.o: $(ENET_SRC)/%.c $(ENET_INC)/enet/*.h | $(ENET_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(ENET_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(TESTGAME_OBJ)/%.$o: $(TESTGAME_SRC)/%.c | $(TESTGAME_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(TESTGAME_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(TESTGAME_OBJ)/%.$o: $(TESTGAME_SRC)/%.m | $(TESTGAME_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(TESTGAME_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(TESTGAME_OBJ)/%.$o: $(TESTGAME_RSRC)/%.rc | $(TESTGAME_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(RC) -i $< -o $@ --include-dir=$(ENGINE_INC) --include-dir=$(TESTGAME_SRC) --include-dir=$(TESTGAME_RSRC) $(RECIPE_RESULT_COMPILE)

$(TESTGAME_OBJ)/%.$o: $(TESTGAME_RSRC)/%.c | $(TESTGAME_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(TESTGAME_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(TESTGAME_RSRC)/%_banner.c: $(TESTGAME_RSRC)/%.bmp
	echo "#include \"gtkpixdata_shim.h\"" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.c | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/rev.$o: $(DUKE3D_SRC)/rev.c | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) $(REVFLAG) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/util/%.c | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.m | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_OBJC) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/%.cpp | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CXX) $(CPPONLYFLAGS) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/misc/%.rc | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(RC) -i $< -o $@ --include-dir=$(ENGINE_INC) --include-dir=$(DUKE3D_SRC) --include-dir=$(DUKE3D_RSRC) -DPOLYMER=$(POLYMER) $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_RSRC)/%.c | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_RSRC)/%_banner.c: $(DUKE3D_RSRC)/%.bmp
	echo "#include \"gtkpixdata_shim.h\"" > $@

$(SW_OBJ)/%.$o: $(SW_SRC)/%.c | $(SW_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(SW_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(SW_OBJ)/%.$o: $(SW_SRC)/%.m | $(SW_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(SW_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(SW_OBJ)/%.$o: $(SW_RSRC)/%.rc | $(SW_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(RC) -i $< -o $@ --include-dir=$(ENGINE_INC) --include-dir=$(SW_SRC) --include-dir=$(SW_RSRC) $(RECIPE_RESULT_COMPILE)

$(SW_OBJ)/%.$o: $(SW_RSRC)/%.c | $(SW_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER) $(COMMONFLAGS) $(COMPILERFLAGS) $(SW_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(SW_RSRC)/%_banner.c: $(SW_RSRC)/%.bmp
	echo "#include \"gtkpixdata_shim.h\"" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

$(ENGINE_OBJ) $(MACT_OBJ) $(AUDIOLIB_OBJ) $(ENET_OBJ) $(TESTGAME_OBJ) $(DUKE3D_OBJ) $(SW_OBJ):
	-mkdir $@ $(DONT_PRINT) $(DONT_FAIL)

## PHONIES

clean: cleanduke3d

# Delete executables, object files created from sources in source/, and
# miscellaneous generated files related to Lunatic on non-Linux.
cleanduke3d:
ifeq ($(PRETTY_OUTPUT),1)
	echo "rm -f $(EDUKE32)$(EXESUFFIX) $(MAPSTER32)$(EXESUFFIX) $(DUKE3D_OBJ)/*.$o $(DUKE3D_MISCDEPS) $(DUKE3D_EDITOR_MISCDEPS)"
endif
	-rm -f $(EDUKE32)$(EXESUFFIX) $(MAPSTER32)$(EXESUFFIX) $(DUKE3D_OBJ)/*.$o $(DUKE3D_MISCDEPS) $(DUKE3D_EDITOR_MISCDEPS)
ifeq ($(PLATFORM),DARWIN)
	-rm -rf EDuke32.app Mapster32.app
endif

cleantest:
	-rm -f $(TESTGAME)$(EXESUFFIX) $(TESTEDITOR)$(EXESUFFIX) $(TESTGAME_OBJ)/*.$o

cleansw:
	-rm -f $(SW)$(EXESUFFIX) $(SW_EDITOR)$(EXESUFFIX) $(SW_OBJ)/*.$o

cleanutils:
	-rm -f $(addsuffix $(EXESUFFIX),$(UTILS) $(GAMEUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(UTILS))) $(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(GAMEUTILS))) $(addsuffix $(EXESUFFIX),$(DXUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(DXUTILS))) $(addsuffix $(EXESUFFIX),$(SDLUTILS)) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(SDLUTILS))) $(UTIL_OBJS_EXP)

veryclean: clean cleanutils
ifeq ($(PRETTY_OUTPUT),1)
	echo "Removed test game and utils objects."
	echo "rm -f $(ENGINE_OBJ)/*.$o $(MACT_OBJ)/*.$o $(AUDIOLIB_OBJ)/*.$o $(ENET_OBJ)/*.$o $(DUKE3D_RSRC)/*banner* $(EBACKTRACEDLL)"
endif
	-rm -f $(ENGINE_OBJ)/*.$o $(MACT_OBJ)/*.$o $(AUDIOLIB_OBJ)/*.$o $(ENET_OBJ)/*.$o $(DUKE3D_RSRC)/*banner* $(EBACKTRACEDLL)

printutils:
	echo "$(addsuffix $(EXESUFFIX),$(UTILS) $(GAMEUTILS))"

printdxutils:
	echo "$(addsuffix $(EXESUFFIX),$(DXUTILS))"

printsdlutils:
	echo "$(addsuffix $(EXESUFFIX),$(SDLUTILS))"

rev: $(DUKE3D_OBJ)/rev.$o
