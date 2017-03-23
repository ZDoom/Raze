#
# EDuke32 Makefile for GNU Make
#

include Common.mak

source=source
DUKE3D=duke3d
DUKE3D_ROOT=$(source)/$(DUKE3D)
DUKE3D_SRC=$(DUKE3D_ROOT)/src
DUKE3D_RSRC=$(DUKE3D_ROOT)/rsrc
ENGINE_ROOT=$(source)/$(ENGINE)
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


# BUILD Engine

ENGINE=build

ENGINE_CFLAGS=-I$(ENGINE_SRC)

ENGINE_OBJ=$(obj)/$(ENGINE)

ENGINE_OBJS = \
    rev \
    baselayer \
    cache1d \
    common \
    compat \
    crc32 \
    defs \
    engine \
    tiles \
    clip \
    2d \
    hash \
    palette \
    polymost \
    texcache \
    dxtfilter \
    hightile \
    textfont \
    smalltextfont \
    kplib \
    lz4 \
    osd \
    pragmas \
    scriptfile \
    mmulti_null \
    mutex \
    xxhash \
    md4 \
    colmatch \
    screenshot \
    mhk \

ENGINE_EDITOR_OBJS = \
    build \
    config \
    defs \

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
    ifeq ($(STARTUP_WINDOW),1)
        ENGINE_EDITOR_OBJS += startosx.editor
    endif
    ifeq ($(SDL_TARGET),1)
        ifneq ($(SDL_FRAMEWORK),0)
            ENGINE_OBJS+=SDLMain
        endif
    endif
endif
ifeq ($(PLATFORM),WINDOWS)
    ENGINE_OBJS+= winbits
    ifeq ($(STARTUP_WINDOW),1)
        ENGINE_EDITOR_OBJS+= startwin.editor
    endif
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
        ifeq ($(STARTUP_WINDOW),1)
            ENGINE_EDITOR_OBJS+= startgtk.editor
        endif
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    ENGINE_OBJS+= winlayer rawinput
endif

ifneq ($(USE_LIBVPX),0)
    ENGINE_OBJS+= animvpx
endif

ENGINE_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(ENGINE_OBJS)))
ENGINE_EDITOR_OBJS_EXP:=$(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(ENGINE_EDITOR_OBJS)))


# MACT

MACT=mact

MACT_ROOT=$(source)/$(MACT)
MACT_SRC=$(MACT_ROOT)/src
MACT_INC=$(MACT_ROOT)/include
MACT_OBJ=$(obj)/$(MACT)

MACT_OBJS = \
    file_lib \
    control \
    keyboard \
    joystick \
    scriplib \
    animlib \

MACT_OBJS_EXP:=$(addprefix $(MACT_OBJ)/,$(addsuffix .$o,$(MACT_OBJS)))


# AudioLib

AUDIOLIB=audiolib

AUDIOLIB_OBJS = \
    drivers \
    fx_man \
    multivoc \
    mix \
    mixst \
    pitch \
    formats \
    vorbis \
    flac \
    xa \
    xmp \
    driver_nosound \

AUDIOLIB_ROOT=$(source)/$(AUDIOLIB)
AUDIOLIB_SRC=$(AUDIOLIB_ROOT)/src
AUDIOLIB_INC=$(AUDIOLIB_ROOT)/include
AUDIOLIB_OBJ=$(obj)/$(AUDIOLIB)

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(MIXERTYPE),WIN)
        AUDIOLIB_OBJS+= driver_directsound
    endif
endif

ifeq ($(MIXERTYPE),SDL)
    ifneq ($(PLATFORM),DARWIN)
        ifneq ($(PLATFORM),WINDOWS)
            ifneq ($(PLATFORM),WII)
                AUDIOLIB_CFLAGS+=`$(PKG_CONFIG) --cflags vorbis`
            endif
        endif
    endif
    AUDIOLIB_OBJS+= driver_sdl
endif

AUDIOLIB_OBJS_EXP:=$(addprefix $(AUDIOLIB_OBJ)/,$(addsuffix .$o,$(AUDIOLIB_OBJS)))


# ENet

ENET=enet

ENET_OBJS = \
    callbacks \
    host \
    list \
    packet \
    peer \
    protocol \
    compress \

ENET_ROOT=$(source)/$(ENET)
ENET_SRC=$(ENET_ROOT)/src
ENET_INC=$(ENET_ROOT)/include
ENET_OBJ=$(obj)/$(ENET)

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

TOOLS=tools

TOOLS_OBJS = \
    compat_tools \

ENGINE_TOOLS_OBJS = \
    compat \
    pragmas \
    kplib \
    cache1d \
    crc32 \
    colmatch \

TOOLS_ROOT=$(source)/$(TOOLS)
TOOLS_SRC=$(TOOLS_ROOT)/src
TOOLS_OBJ=$(obj)/$(TOOLS)

TOOLS_CFLAGS=$(ENGINE_CFLAGS)

TOOLS_TARGETS= \
    kextract \
    kgroup \
    transpal \
    wad2art \
    wad2map \
    kmd2tool \
    md2tool \
    generateicon \
    cacheinfo \
    arttool \
    givedepth \
    mkpalette \
    unpackssi \
    bsuite \
    ivfrate \
    map2stl \

ifeq ($(PLATFORM),WINDOWS)
    TOOLS_TARGETS+= enumdisplay getdxdidf
endif
ifeq ($(RENDERTYPE),SDL)
    TOOLS_TARGETS+= makesdlkeytrans
endif

ifeq ($(PLATFORM),DARWIN)
    TOOLS_OBJS += osxbits
endif

TOOLS_OBJS_EXP:=$(addprefix $(TOOLS_OBJ)/,$(addsuffix .$o,$(TOOLS_OBJS))) $(addprefix $(ENGINE_OBJ)/,$(addsuffix .$o,$(ENGINE_TOOLS_OBJS)))


# KenBuild (Test Game)

KENBUILD=kenbuild
kenbuild=KENBUILD

KENBUILD_ROOT=$(source)/$(KENBUILD)
KENBUILD_SRC=$(KENBUILD_ROOT)/src
KENBUILD_RSRC=$(KENBUILD_ROOT)/rsrc
KENBUILD_OBJ=$(obj)/$(KENBUILD)

KENBUILD_CFLAGS=-I$(KENBUILD_SRC)

KENBUILD_GAME ?= ekenbuild
KENBUILD_EDITOR ?= ekenbuild-editor

KENBUILD_GAME_PROPER ?= EKenBuild
KENBUILD_EDITOR_PROPER ?= EKenBuild Editor

KENBUILD_GAME_OBJS = \
    game \
    sound_stub \
    common \
    config \

KENBUILD_EDITOR_OBJS = \
    bstub \
    common \

ifeq ($(RENDERTYPE),SDL)
    ifeq (1,$(HAVE_GTK2))
        KENBUILD_GAME_OBJS+= game_banner startgtk.game
        KENBUILD_EDITOR_OBJS+= build_banner
    endif

    KENBUILD_GAME_OBJS+= game_icon
    KENBUILD_EDITOR_OBJS+= build_icon
endif
ifeq ($(PLATFORM),WINDOWS)
    KENBUILD_GAME_OBJS+= gameres startwin.game
    KENBUILD_EDITOR_OBJS+= buildres
endif

ifeq ($(PLATFORM),DARWIN)
    ifeq ($(STARTUP_WINDOW),1)
        KENBUILD_GAME_OBJS += StartupWinController.game
    endif
endif

KENBUILD_GAME_OBJS_EXP:=$(addprefix $(KENBUILD_OBJ)/,$(addsuffix .$o,$(KENBUILD_GAME_OBJS)))
KENBUILD_EDITOR_OBJS_EXP:=$(addprefix $(KENBUILD_OBJ)/,$(addsuffix .$o,$(KENBUILD_EDITOR_OBJS)))


# Duke Nukem 3D

DUKE3D=duke3d
duke3d=DUKE3D

DUKE3D_CFLAGS=-I$(DUKE3D_SRC)

DUKE3D_GAME_LDFLAGS=
DUKE3D_EDITOR_LDFLAGS=

DUKE3D_GAME_STRIPFLAGS=
DUKE3D_EDITOR_STRIPFLAGS=

DUKE3D_OBJ=$(obj)/$(DUKE3D)

DUKE3D_GAME ?= eduke32
DUKE3D_EDITOR ?= mapster32

DUKE3D_GAME_PROPER ?= EDuke32
DUKE3D_EDITOR_PROPER ?= Mapster32

COMMON_EDITOR_OBJS = \
    m32common \
    m32def \
    m32exec \
    m32vars \

DUKE3D_GAME_OBJS = \
    game \
    global \
    actors \
    gamedef \
    gameexec \
    gamevars \
    player \
    premap \
    sector \
    anim \
    animsounds \
    common \
    config \
    demo \
    input \
    menus \
    namesdyn \
    net \
    savegame \
    rts \
    osdfuncs \
    osdcmds \
    grpscan \
    sounds \
    soundsdyn \
    cheats \
    sbar \
    screentext \
    screens \
    cmdline \

DUKE3D_EDITOR_OBJS = \
    astub \
    common \
    grpscan \
    sounds_mapster32 \

DUKE3D_GAME_MISCDEPS=
DUKE3D_EDITOR_MISCDEPS=

## Lunatic devel
LUNATIC_LUA_PREFIX = luaJIT_BC_
ifneq (0,$(LUNATIC))
    # Lunatic object base names. These are not used in targets directly.
    LUNATIC_LUA_OBJS = \
        defs_common \
        engine_maptext \
        engine \
        bcarray \
        bcheck \
        bitar \
        xmath \
        v \
        dump \
        dis_x86 \
        dis_x64 \

    LUNATIC_GAME_LUA_OBJS = \
        _defs_game \
        con_lang \
        lunacon \
        randgen \
        stat \
        control \
        savegame \
        fs \

    LUNATIC_EDITOR_LUA_OBJS = \
        _defs_editor \

    LUNATIC_GAME_OBJS = \
        lunatic_game \

    LUNATIC_EDITOR_OBJS = \
        lunatic_editor \

    # TODO: remove debugging modules from release build

    ifneq ($(PLATFORM),WINDOWS)
        # On non-Windows, we expect to have liblpeg.a (or a symlink to it) in source/.
        # On Windows, it will reside in platform/Windows/lib/32/ or lib/64/.
        LIBDIRS+= -L$(source)
        ifeq ($(realpath $(source)/liblpeg.a),)
            # XXX: This cripples "make clean" etc. too, but IMO it's better than warning.
            $(error "liblpeg.a not found in $(realpath $(source))")
        endif
    endif
    LIBS+= -llpeg

    # now, take care of having the necessary symbols (sector, wall, etc.) in the
    # executable no matter what the debugging level

    ifeq ($(PLATFORM),DARWIN)
        # strip on OSX says: removing global symbols from a final linked no longer supported.
        #                    Use -exported_symbols_list at link time when building
        # But, following _their_ directions does not give us the symbols! wtf?
        # Instead of using -alias_list and -exported_symbols_list, prevent stripping them.
        DUKE3D_GAME_STRIPFLAGS+= -s $(DUKE3D_OBJ)/lunatic_dynsymlist_game_osx
        DUKE3D_EDITOR_STRIPFLAGS+= -s $(DUKE3D_OBJ)/lunatic_dynsymlist_editor_osx

        DUKE3D_GAME_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_game_osx
        DUKE3D_EDITOR_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_editor_osx
        LINKERFLAGS+= -pagezero_size 10000 -image_base 100000000
    endif
    ifeq ($(PLATFORM),WINDOWS)
        override STRIP=
        DUKE3D_GAME_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_game.def
        DUKE3D_EDITOR_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_editor.def
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        override STRIP=
        DUKE3D_GAME_LDFLAGS+= -Wl,--dynamic-list=$(DUKE3D_SRC)/lunatic/dynsymlist_game.lds
        DUKE3D_EDITOR_LDFLAGS+= -Wl,--dynamic-list=$(DUKE3D_SRC)/lunatic/dynsymlist_editor.lds
    endif
endif

ifeq ($(SUBPLATFORM),LINUX)
    ifneq (0,$(HAVE_XMP))
        LIBS += -lxmp-lite
    endif
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),BSD)
    ifneq (0,$(HAVE_XMP))
        LIBS += -lxmp-lite
    endif
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lexecinfo
endif

ifeq ($(PLATFORM),DARWIN)
    ifneq (0,$(HAVE_XMP))
        LIBS += -lxmp-lite
    endif
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm \
            -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
            -Wl,-framework,CoreMIDI -Wl,-framework,AudioUnit \
            -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL
    ifneq (00,$(DARWIN9)$(DARWIN10))
        LIBS += -Wl,-framework,QuickTime -lm
    endif

    ifeq ($(STARTUP_WINDOW),1)
        DUKE3D_GAME_OBJS += GrpFile.game GameListSource.game startosx.game
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    ifneq (0,$(HAVE_XMP))
        LIBS += -lxmp-lite
    endif
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
    DUKE3D_GAME_OBJS+= gameres winbits
    DUKE3D_EDITOR_OBJS+= buildres
    ifeq ($(STARTUP_WINDOW),1)
        DUKE3D_GAME_OBJS+= startwin.game
    endif
    ifeq ($(MIXERTYPE),WIN)
        LIBS+= -ldsound
        MIDI_OBJS=music midi mpu401
    endif
endif

ifeq ($(PLATFORM),WII)
    LIBS += -lvorbisidec
endif

ifeq ($(RENDERTYPE),SDL)
    ifeq (11,$(HAVE_GTK2)$(STARTUP_WINDOW))
        DUKE3D_GAME_OBJS+= game_banner startgtk.game
        DUKE3D_EDITOR_OBJS+= build_banner
    endif

    DUKE3D_GAME_OBJS+= game_icon
    DUKE3D_EDITOR_OBJS+= build_icon
endif
ifeq ($(MIXERTYPE),SDL)
    MIDI_OBJS=sdlmusic
endif

## Construct file names of object files

COMMON_EDITOR_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(COMMON_EDITOR_OBJS))) $(ENGINE_EDITOR_OBJS_EXP)

MIDI_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(MIDI_OBJS)))

DUKE3D_GAME_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_GAME_OBJS))) $(MIDI_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(MACT_OBJS_EXP) $(ENET_TARGET)
DUKE3D_EDITOR_OBJS_EXP:=$(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(DUKE3D_EDITOR_OBJS))) $(AUDIOLIB_OBJS_EXP)

ifneq (0,$(LUNATIC))
    DUKE3D_GAME_OBJS_EXP+= $(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(LUNATIC_GAME_OBJS) $(addprefix $(LUNATIC_LUA_PREFIX),$(LUNATIC_LUA_OBJS) $(LUNATIC_GAME_LUA_OBJS))))
    DUKE3D_EDITOR_OBJS_EXP+= $(addprefix $(DUKE3D_OBJ)/,$(addsuffix .$o,$(LUNATIC_EDITOR_OBJS) $(addprefix $(LUNATIC_LUA_PREFIX),$(LUNATIC_LUA_OBJS) $(LUNATIC_EDITOR_LUA_OBJS))))
endif

# Shadow Warrior

SW=sw
sw=SW

SW_ROOT=$(source)/$(SW)
SW_SRC=$(SW_ROOT)/src
SW_RSRC=$(SW_ROOT)/rsrc
SW_OBJ=$(obj)/$(SW)

SW_CFLAGS=-I$(SW_SRC)

SW_GAME ?= voidsw
SW_EDITOR ?= voidsw-editor

SW_GAME_PROPER ?= VoidSW
SW_EDITOR_PROPER ?= VoidSW Editor

SW_GAME_OBJS = \
    actor \
    ai \
    anim \
    border \
    break \
    bunny \
    cache \
    cheats \
    colormap \
    common \
    config \
    console \
    coolg \
    coolie \
    copysect \
    demo \
    draw \
    eel \
    game \
    girlninj \
    goro \
    grpscan \
    hornet \
    interp \
    interpsh \
    inv \
    jplayer \
    jsector \
    jweapon \
    lava \
    light \
    mclip \
    mdastr \
    menus \
    miscactr \
    morph \
    net \
    ninja \
    panel \
    player \
    predict \
    quake \
    ripper \
    ripper2 \
    rooms \
    rotator \
    rts \
    save \
    scrip2 \
    sector \
    serp \
    setup \
    skel \
    skull \
    slidor \
    sounds \
    spike \
    sprite \
    sumo \
    swconfig \
    sync \
    text \
    track \
    vator \
    vis \
    wallmove \
    warp \
    weapon \
    zilla \
    zombie \
    saveable \

SW_EDITOR_OBJS = \
    jnstub \
    brooms \
    bldscript \
    jbhlp \
    colormap \
    grpscan \
    common \

ifeq ($(RENDERTYPE),SDL)
    ifeq (1,$(HAVE_GTK2))
        SW_GAME_OBJS+= game_banner startgtk.game
        SW_EDITOR_OBJS+= build_banner
    endif

    SW_GAME_OBJS+= game_icon
    SW_EDITOR_OBJS+= game_icon
endif
ifeq ($(PLATFORM),WINDOWS)
    SW_GAME_OBJS+= gameres startwin.game
    SW_EDITOR_OBJS+= buildres
endif

SW_GAME_OBJS_EXP:=$(addprefix $(SW_OBJ)/,$(addsuffix .$o,$(SW_GAME_OBJS))) $(MIDI_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(MACT_OBJS_EXP)
SW_EDITOR_OBJS_EXP:=$(addprefix $(SW_OBJ)/,$(addsuffix .$o,$(SW_EDITOR_OBJS))) $(AUDIOLIB_OBJS_EXP)


### component definitions end
### targets begin

GAMES := \
    KENBUILD \
    DUKE3D \
    SW \

LIBRARIES := \
    ENGINE \
    AUDIOLIB \
    MACT \
    ENET \

COMPONENTS = \
    $(GAMES) \
    $(LIBRARIES) \
    TOOLS \

ROLES = \
    GAME \
    EDITOR \


ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: \
    all \
    $(foreach j,$(foreach i,$(GAMES),$($i)) test utils tools,$j clean$j) \
    veryclean \
    clean \
    printutils \
    printtools \
    rev \
    $(ENGINE_OBJ)/rev.$o \

.SUFFIXES:
.SECONDEXPANSION:


# TARGETS

all: duke3d

start:
	$(BUILD_STARTED)

tools: $(addsuffix $(EXESUFFIX),$(TOOLS_TARGETS)) | start
	@ls -l $^

$(foreach i,$(GAMES),$($i)): $$(foreach i,$(ROLES),$$($$($$@)_$$i)$(EXESUFFIX)) | start
	@ls -l $^

ebacktrace: $(EBACKTRACEDLL) | start
	@ls -l $^

ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
%$(DOLSUFFIX): %$(EXESUFFIX)
endif
endif

define BUILDRULE

$$($1_$2)$$(EXESUFFIX): $$($1_$2_OBJS_EXP) $$(COMMON_$2_OBJS_EXP) $$(ENGINE_OBJS_EXP) $$($1_$2_MISCDEPS)
	$$(LINK_STATUS)
	$$(RECIPE_IF) $$(LINKER) -o $$@ $$^ $$(GUI_LIBS) $$($1_$2_LDFLAGS) $$(LIBDIRS) $$(LIBS) $$(RECIPE_RESULT_LINK)
ifeq ($$(PLATFORM),WII)
ifneq ($$(ELF2DOL),)
	$$(ELF2DOL) $$@ $$($1_$2)$$(DOLSUFFIX)
endif
endif
ifneq ($$(STRIP),)
	$$(STRIP) $$@ $$($1_$2_STRIPFLAGS)
endif
ifeq ($$(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/$$($1_$2_PROPER).app" "./"
	mkdir -p "$$($1_$2_PROPER).app/Contents/MacOS"
	cp -f "$$($1_$2)$$(EXESUFFIX)" "$$($1_$2_PROPER).app/Contents/MacOS/"
endif

endef

$(foreach i,$(GAMES),$(foreach j,$(ROLES),$(eval $(call BUILDRULE,$i,$j))))


include $(ENGINE_ROOT)/Dependencies.mak
include $(DUKE3D_ROOT)/Dependencies.mak
include $(SW_ROOT)/Dependencies.mak


# RULES

$(EBACKTRACEDLL): platform/Windows/src/backtrace.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CC) $(CONLYFLAGS) -O2 -ggdb -shared -Wall -Wextra -static-libgcc -I$(ENGINE_INC) -o $@ $^ -lbfd -liberty -limagehlp $(RECIPE_RESULT_COMPILE)

libcache1d$(DLLSUFFIX): $(ENGINE_SRC)/cache1d.cpp
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_C) -DCACHE1D_COMPRESS_ONLY -shared -fPIC $< -o $@ $(RECIPE_RESULT_COMPILE)

%$(EXESUFFIX): $(TOOLS_OBJ)/%.$o $(TOOLS_OBJS_EXP)
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) $(RECIPE_RESULT_LINK)

enumdisplay$(EXESUFFIX): $(TOOLS_OBJ)/enumdisplay.$o
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) -lgdi32 $(RECIPE_RESULT_LINK)
getdxdidf$(EXESUFFIX): $(TOOLS_OBJ)/getdxdidf.$o
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) -ldinput $(RECIPE_RESULT_LINK)


#### Lunatic

# Create object files directly with luajit
$(DUKE3D_OBJ)/$(LUNATIC_LUA_PREFIX)%.$o: $(DUKE3D_SRC)/lunatic/%.lua | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(LUAJIT) -bg $(LUAJIT_BCOPTS) $< $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(DUKE3D_SRC)/lunatic/%.cpp | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

# List of exported symbols, OS X
$(DUKE3D_OBJ)/lunatic_%_osx: $(DUKE3D_SRC)/lunatic/%.lds | $(DUKE3D_OBJ)
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

# List of exported symbols, Windows
$(DUKE3D_OBJ)/lunatic_%.def: $(DUKE3D_SRC)/lunatic/%.lds | $(DUKE3D_OBJ)
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@


#### Main Rules

define OBJECTRULES

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.nasm | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(AS) $$(ASFLAGS) $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.yasm | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(AS) $$(ASFLAGS) $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.c | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.cpp | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_CXX) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.m | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_OBJC) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_SRC)/%.mm | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_OBJCXX) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

# cosmetic stuff

$$($1_OBJ)/%.$$o: $$($1_RSRC)/%.rc | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(RC) -i $$< -o $$@ --include-dir=$$(ENGINE_INC) --include-dir=$$($1_SRC) --include-dir=$$($1_RSRC) -DPOLYMER=$$(POLYMER) $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_RSRC)/%.c | $$($1_OBJ)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%.$$o: $$($1_OBJ)/%.c
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_CFLAGS) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_OBJ)/%_banner.c: $$($1_RSRC)/%.bmp | $$($1_OBJ)
	echo "#include \"gtkpixdata_shim.h\"" > $$@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $$^ | sed 's/load_inc//' >> $$@

endef

$(foreach i,$(COMPONENTS),$(eval $(call OBJECTRULES,$i)))


#### Other special cases

# Comment out the following rule to debug a-c.o
$(ENGINE_OBJ)/a-c.$o: $(ENGINE_SRC)/a-c.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(subst -O$(OPTLEVEL),-O2,$(subst $(CLANG_DEBUG_FLAGS),,$(COMPILER_CXX))) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/rev.$o: $(ENGINE_SRC)/rev.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(ENGINE_CFLAGS) $(REVFLAG) -c $< -o $@ $(RECIPE_RESULT_COMPILE)


#### Directories

$(obj):
	-mkdir $@ $(DONT_PRINT) $(DONT_FAIL)

$(foreach i,$(COMPONENTS),$($i_OBJ)): | $(obj)
	-mkdir $@ $(DONT_PRINT) $(DONT_FAIL)


## PHONIES

$(foreach i,$(GAMES),clean$($i)):
	-rm -f $(foreach i,$(ROLES),$($($(subst clean,,$@))_$i)$(EXESUFFIX))
ifeq ($(PLATFORM),DARWIN)
	-rm -rf $(foreach i,$(ROLES),"$($($(subst clean,,$@))_$i_PROPER).app")
endif

cleantools:
	-rm -f $(addsuffix $(EXESUFFIX),$(TOOLS_TARGETS))

clean: cleanduke3d cleantools
	-rm -rf $(obj)/
	-rm -f $(EBACKTRACEDLL)

printtools:
	echo "$(addsuffix $(EXESUFFIX),$(TOOLS_TARGETS))"

rev: $(ENGINE_OBJ)/rev.$o


# Compatibility

test: kenbuild
utils: tools
printutils: printtools
veryclean: clean
cleanutils: cleantools
cleantest: cleankenbuild
