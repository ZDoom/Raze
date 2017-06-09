#
# EDuke32 Makefile for GNU Make
#

include Common.mak

### File Extensions
o=o

### Directories
source=source
obj=obj

### Functions
define expandobjs
$$(addprefix $1,$$(addsuffix .$$o,$$(basename $2)))
endef

COMPILERFLAGS += -I$(ENGINE_INC) -I$(MACT_INC) -I$(AUDIOLIB_INC) -I$(ENET_INC)


##### Component Definitions

#### EBacktrace

ifndef EBACKTRACEDLL
    EBACKTRACEDLL = ebacktrace1.dll
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        EBACKTRACEDLL = ebacktrace1-64.dll
    endif
endif


#### BUILD Engine

ENGINE=build

ENGINE_CFLAGS=-I$(ENGINE_SRC)

ENGINE_ROOT=$(source)/$(ENGINE)
ENGINE_SRC=$(ENGINE_ROOT)/src
ENGINE_INC=$(ENGINE_ROOT)/include
ENGINE_OBJ=$(obj)/$(ENGINE)

ENGINE_OBJS = \
    rev.cpp \
    baselayer.cpp \
    cache1d.cpp \
    common.cpp \
    compat.cpp \
    crc32.cpp \
    defs.cpp \
    engine.cpp \
    tiles.cpp \
    clip.cpp \
    2d.cpp \
    hash.cpp \
    palette.cpp \
    polymost.cpp \
    texcache.cpp \
    dxtfilter.cpp \
    hightile.cpp \
    textfont.cpp \
    smalltextfont.cpp \
    kplib.cpp \
    lz4.c \
    osd.cpp \
    pragmas.cpp \
    scriptfile.cpp \
    mmulti_null.cpp \
    mutex.cpp \
    xxhash.c \
    md4.cpp \
    colmatch.cpp \
    screenshot.cpp \
    mhk.cpp \

ENGINE_EDITOR_OBJS = \
    build.cpp \
    config.cpp \
    defs.cpp \

ENGINE_TOOLS_OBJS = \
    compat.cpp \
    pragmas.cpp \
    kplib.cpp \
    cache1d.cpp \
    crc32.cpp \
    colmatch.cpp \

ifeq (0,$(NOASM))
  ENGINE_OBJS+= a.nasm
else
  ENGINE_OBJS+= a-c.cpp
  ifneq (0,$(USE_ASM64))
    ENGINE_OBJS+= a64.yasm
  endif
endif
ifeq (1,$(USE_OPENGL))
    ENGINE_OBJS+= glbuild.cpp voxmodel.cpp mdsprite.cpp
    ifeq (1,$(POLYMER))
        ENGINE_OBJS+= polymer.cpp
    endif
endif
ifneq (0,$(LUNATIC))
    ENGINE_OBJS+= lunatic.cpp
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINE_OBJS += osxbits.mm
    ifeq ($(STARTUP_WINDOW),1)
        ENGINE_EDITOR_OBJS += startosx.editor.mm
    endif
    ifeq ($(SDL_TARGET),1)
        ifneq ($(SDL_FRAMEWORK),0)
            ENGINE_OBJS+=SDLMain.mm
        endif
    endif
endif
ifeq ($(PLATFORM),WINDOWS)
    ENGINE_OBJS+= winbits.cpp
    ifeq ($(STARTUP_WINDOW),1)
        ENGINE_EDITOR_OBJS+= startwin.editor.cpp
    endif
endif
ifeq ($(PLATFORM),WII)
    ENGINE_OBJS+= wiibits.cpp
    LINKERFLAGS+= -Wl,-wrap,c_default_exceptionhandler
endif
ifeq ($(RENDERTYPE),SDL)
    ENGINE_OBJS+= sdlayer.cpp

    ifeq (1,$(HAVE_GTK2))
        ENGINE_OBJS+= gtkbits.cpp dynamicgtk.cpp
        ifeq ($(STARTUP_WINDOW),1)
            ENGINE_EDITOR_OBJS+= startgtk.editor.cpp
        endif
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    ENGINE_OBJS+= winlayer.cpp rawinput.cpp
endif

ifneq ($(USE_LIBVPX),0)
    ENGINE_OBJS+= animvpx.cpp
endif

ENGINE_SRCS:=$(addprefix $(ENGINE_SRC)/,$(ENGINE_OBJS))
ENGINE_EDITOR_SRCS:=$(addprefix $(ENGINE_SRC)/,$(ENGINE_EDITOR_OBJS))
ENGINE_OBJS_EXP:=$(call expandobjs,$(ENGINE_OBJ)/,$(ENGINE_OBJS))
ENGINE_EDITOR_OBJS_EXP:=$(call expandobjs,$(ENGINE_OBJ)/,$(ENGINE_EDITOR_OBJS))


#### MACT

MACT=mact

MACT_ROOT=$(source)/$(MACT)
MACT_SRC=$(MACT_ROOT)/src
MACT_INC=$(MACT_ROOT)/include
MACT_OBJ=$(obj)/$(MACT)

MACT_OBJS = \
    file_lib.cpp \
    control.cpp \
    keyboard.cpp \
    joystick.cpp \
    scriplib.cpp \
    animlib.cpp \

MACT_SRCS:=$(addprefix $(MACT_SRC)/,$(MACT_OBJS))
MACT_OBJS_EXP:=$(call expandobjs,$(MACT_OBJ)/,$(MACT_OBJS))


#### AudioLib

AUDIOLIB=audiolib

AUDIOLIB_OBJS = \
    drivers.cpp \
    fx_man.cpp \
    multivoc.cpp \
    mix.cpp \
    mixst.cpp \
    pitch.cpp \
    formats.cpp \
    vorbis.cpp \
    flac.cpp \
    xa.cpp \
    xmp.cpp \
    driver_nosound.cpp \

AUDIOLIB_ROOT=$(source)/$(AUDIOLIB)
AUDIOLIB_SRC=$(AUDIOLIB_ROOT)/src
AUDIOLIB_INC=$(AUDIOLIB_ROOT)/include
AUDIOLIB_OBJ=$(obj)/$(AUDIOLIB)

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(MIXERTYPE),WIN)
        AUDIOLIB_OBJS+= driver_directsound.cpp
    endif
endif

ifeq ($(MIXERTYPE),SDL)
    ifeq (,$(filter $(PLATFORM),DARWIN WINDOWS WII))
        AUDIOLIB_CFLAGS+=`$(PKG_CONFIG) --cflags vorbis`
    endif
    AUDIOLIB_OBJS+= driver_sdl.cpp
endif

AUDIOLIB_SRCS:=$(addprefix $(AUDIOLIB_SRC)/,$(AUDIOLIB_OBJS))
AUDIOLIB_OBJS_EXP:=$(call expandobjs,$(AUDIOLIB_OBJ)/,$(AUDIOLIB_OBJS))


#### ENet

ENET=enet

ENET_OBJS = \
    callbacks.c \
    host.c \
    list.c \
    packet.c \
    peer.c \
    protocol.c \
    compress.c \

ENET_ROOT=$(source)/$(ENET)
ENET_SRC=$(ENET_ROOT)/src
ENET_INC=$(ENET_ROOT)/include
ENET_OBJ=$(obj)/$(ENET)

ENET_CFLAGS=

ifeq ($(PLATFORM),WINDOWS)
    ENET_OBJS += win32.c
else
    ENET_OBJS += unix.c
    ENET_CFLAGS += -DHAS_SOCKLEN_T
endif

ENET_SRCS:=$(addprefix $(ENET_SRC)/,$(ENET_OBJS))
ENET_OBJS_EXP:=$(call expandobjs,$(ENET_OBJ)/,$(ENET_OBJS))

ifeq ($(NETCODE),0)
    override ENET_SRCS:=
    override ENET_OBJS_EXP:=
endif


#### Tools

TOOLS=tools

TOOLS_OBJS = \
    compat_tools.cpp \

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
    TOOLS_OBJS += osxbits.mm
endif

TOOLS_SRCS:=$(addprefix $(TOOLS_SRC)/,$(TOOLS_OBJS)) $(addprefix $(ENGINE_SRC)/,$(ENGINE_TOOLS_OBJS))
TOOLS_OBJS_EXP:=$(call expandobjs,$(TOOLS_OBJ)/,$(TOOLS_OBJS)) $(call expandobjs,$(ENGINE_OBJ)/,$(ENGINE_TOOLS_OBJS))


#### KenBuild (Test Game)

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
    game.cpp \
    sound_stub.cpp \
    common.cpp \
    config.cpp \

KENBUILD_EDITOR_OBJS = \
    bstub.cpp \
    common.cpp \

KENBUILD_GAME_RSRC_OBJS =
KENBUILD_EDITOR_RSRC_OBJS =

ifeq (1,$(HAVE_GTK2))
    KENBUILD_GAME_OBJS+= startgtk.game.cpp
    KENBUILD_GAME_RSRC_OBJS+= game_banner.c
    KENBUILD_EDITOR_RSRC_OBJS+= build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    KENBUILD_GAME_RSRC_OBJS+= game_icon.c
    KENBUILD_EDITOR_RSRC_OBJS+= build_icon.c
endif
ifeq ($(PLATFORM),WINDOWS)
    KENBUILD_GAME_OBJS+= startwin.game.cpp
    KENBUILD_GAME_RSRC_OBJS+= gameres.rc
    KENBUILD_EDITOR_RSRC_OBJS+= buildres.rc
endif

ifeq ($(PLATFORM),DARWIN)
    ifeq ($(STARTUP_WINDOW),1)
        KENBUILD_GAME_OBJS += StartupWinController.game.mm
    endif
endif

KENBUILD_GAME_SRCS:=$(addprefix $(KENBUILD_SRC)/,$(KENBUILD_GAME_OBJS))
KENBUILD_EDITOR_SRCS:=$(addprefix $(KENBUILD_SRC)/,$(KENBUILD_EDITOR_OBJS))
KENBUILD_GAME_OBJS_EXP:=$(call expandobjs,$(KENBUILD_OBJ)/,$(KENBUILD_GAME_OBJS) $(KENBUILD_GAME_RSRC_OBJS))
KENBUILD_EDITOR_OBJS_EXP:=$(call expandobjs,$(KENBUILD_OBJ)/,$(KENBUILD_EDITOR_OBJS) $(KENBUILD_EDITOR_RSRC_OBJS))

KENBUILD_GAME_SRCS_TARGET=$(KENBUILD_GAME_SRCS)
KENBUILD_EDITOR_SRCS_TARGET=$(KENBUILD_EDITOR_SRCS)
KENBUILD_GAME_OBJS_TARGET=$(KENBUILD_GAME_OBJS_EXP)
KENBUILD_EDITOR_OBJS_TARGET=$(KENBUILD_EDITOR_OBJS_EXP)


#### Duke Nukem 3D

DUKE3D=duke3d
duke3d=DUKE3D

DUKE3D_CFLAGS=-I$(DUKE3D_SRC)

DUKE3D_GAME_LDFLAGS=
DUKE3D_EDITOR_LDFLAGS=

DUKE3D_GAME_STRIPFLAGS=
DUKE3D_EDITOR_STRIPFLAGS=

DUKE3D_ROOT=$(source)/$(DUKE3D)
DUKE3D_SRC=$(DUKE3D_ROOT)/src
DUKE3D_RSRC=$(DUKE3D_ROOT)/rsrc
DUKE3D_OBJ=$(obj)/$(DUKE3D)

DUKE3D_GAME ?= eduke32
DUKE3D_EDITOR ?= mapster32

ifneq (,$(APPBASENAME))
    DUKE3D_GAME = $(APPBASENAME)
endif

DUKE3D_GAME_PROPER ?= EDuke32
DUKE3D_EDITOR_PROPER ?= Mapster32

DUKE3D_COMMON_EDITOR_OBJS = \
    m32common.cpp \
    m32def.cpp \
    m32exec.cpp \
    m32vars.cpp \

DUKE3D_GAME_OBJS = \
    game.cpp \
    global.cpp \
    actors.cpp \
    gamedef.cpp \
    gameexec.cpp \
    gamevars.cpp \
    player.cpp \
    premap.cpp \
    sector.cpp \
    anim.cpp \
    animsounds.cpp \
    common.cpp \
    config.cpp \
    demo.cpp \
    input.cpp \
    menus.cpp \
    namesdyn.cpp \
    net.cpp \
    savegame.cpp \
    rts.cpp \
    osdfuncs.cpp \
    osdcmds.cpp \
    grpscan.cpp \
    sounds.cpp \
    soundsdyn.cpp \
    cheats.cpp \
    sbar.cpp \
    screentext.cpp \
    screens.cpp \
    cmdline.cpp \

DUKE3D_EDITOR_OBJS = \
    astub.cpp \
    common.cpp \
    grpscan.cpp \
    sounds_mapster32.cpp \

DUKE3D_GAME_RSRC_OBJS =
DUKE3D_EDITOR_RSRC_OBJS =

DUKE3D_GAME_MISCDEPS=
DUKE3D_EDITOR_MISCDEPS=
DUKE3D_GAME_ORDERONLYDEPS=
DUKE3D_EDITOR_ORDERONLYDEPS=

## Lunatic devel
LUNATIC_SRC=$(DUKE3D_SRC)/lunatic
LUNATIC_OBJ=$(DUKE3D_OBJ)

ifneq (0,$(LUNATIC))
    COMPILERFLAGS += -I$(LUNATIC_SRC) -DLUNATIC

    # Determine size of _defs*.lua bytecode once.
    ifndef DEFS_BC_SIZE
        DEFS_BC_SIZE := $(shell $(LUAJIT) -bg -t h $(LUNATIC_SRC)/_defs_game.lua -)
        DEFS_BC_SIZE := $(word 3, $(DEFS_BC_SIZE))
    endif
    ifndef DEFS_M32_BC_SIZE
        DEFS_M32_BC_SIZE := $(shell $(LUAJIT) -bg -t h $(LUNATIC_SRC)/_defs_editor.lua -)
        DEFS_M32_BC_SIZE := $(word 3, $(DEFS_M32_BC_SIZE))
    endif
    DUKE3D_CFLAGS += -DLUNATIC_DEFS_BC_SIZE=$(DEFS_BC_SIZE) -DLUNATIC_DEFS_M32_BC_SIZE=$(DEFS_M32_BC_SIZE)

    # Lunatic object base names. These are not used in targets directly.
    LUNATIC_LUA_OBJS = \
        defs_common.lua \
        engine_maptext.lua \
        engine.lua \
        bcarray.lua \
        bcheck.lua \
        bitar.lua \
        xmath.lua \
        v.lua \
        dump.lua \
        dis_x86.lua \
        dis_x64.lua \

    LUNATIC_GAME_LUA_OBJS = \
        _defs_game.lua \
        con_lang.lua \
        lunacon.lua \
        randgen.lua \
        stat.lua \
        control.lua \
        lunasave.lua \
        fs.lua \

    LUNATIC_EDITOR_LUA_OBJS = \
        _defs_editor.lua \

    LUNATIC_GAME_OBJS = \
        lunatic_game.cpp \

    LUNATIC_EDITOR_OBJS = \
        lunatic_editor.cpp \

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

        DUKE3D_GAME_ORDERONLYDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_game_osx
        DUKE3D_EDITOR_ORDERONLYDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_editor_osx
        LINKERFLAGS+= -pagezero_size 10000 -image_base 100000000
    endif
    ifeq ($(PLATFORM),WINDOWS)
        override STRIP=
        DUKE3D_GAME_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_game.def
        DUKE3D_EDITOR_MISCDEPS+= $(DUKE3D_OBJ)/lunatic_dynsymlist_editor.def
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        override STRIP=
        DUKE3D_GAME_LDFLAGS+= -Wl,--dynamic-list=$(LUNATIC_SRC)/dynsymlist_game.lds
        DUKE3D_EDITOR_LDFLAGS+= -Wl,--dynamic-list=$(LUNATIC_SRC)/dynsymlist_editor.lds
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
        DUKE3D_GAME_OBJS += GrpFile.game.mm GameListSource.game.mm startosx.game.mm
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    ifneq (0,$(HAVE_XMP))
        LIBS += -lxmp-lite
    endif
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
    DUKE3D_GAME_OBJS+= winbits.cpp
    DUKE3D_GAME_RSRC_OBJS+= gameres.rc
    DUKE3D_EDITOR_RSRC_OBJS+= buildres.rc
    ifeq ($(STARTUP_WINDOW),1)
        DUKE3D_GAME_OBJS+= startwin.game.cpp
    endif
    ifeq ($(MIXERTYPE),WIN)
        LIBS+= -ldsound
        DUKE3D_COMMON_MIDI_OBJS=music.cpp midi.cpp mpu401.cpp
    endif
endif

ifeq ($(PLATFORM),WII)
    LIBS += -lvorbisidec
endif

ifeq (11,$(HAVE_GTK2)$(STARTUP_WINDOW))
    DUKE3D_GAME_OBJS+= startgtk.game.cpp
    DUKE3D_GAME_RSRC_OBJS+= game_banner.c
    DUKE3D_EDITOR_RSRC_OBJS+= build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    DUKE3D_GAME_RSRC_OBJS+= game_icon.c
    DUKE3D_EDITOR_RSRC_OBJS+= build_icon.c
endif
ifeq ($(MIXERTYPE),SDL)
    DUKE3D_COMMON_MIDI_OBJS=sdlmusic.cpp
endif

DUKE3D_COMMON_EDITOR_SRCS:=$(addprefix $(DUKE3D_SRC)/,$(DUKE3D_COMMON_EDITOR_OBJS))
DUKE3D_COMMON_EDITOR_OBJS_EXP:=$(call expandobjs,$(DUKE3D_OBJ)/,$(DUKE3D_COMMON_EDITOR_OBJS))

DUKE3D_COMMON_EDITOR_SRCS_TARGET=$(DUKE3D_COMMON_EDITOR_SRCS) $(ENGINE_EDITOR_SRCS)
DUKE3D_COMMON_EDITOR_OBJS_TARGET=$(DUKE3D_COMMON_EDITOR_OBJS_EXP) $(ENGINE_EDITOR_OBJS_EXP)

DUKE3D_COMMON_MIDI_SRCS:=$(addprefix $(DUKE3D_SRC)/,$(DUKE3D_COMMON_MIDI_OBJS))
DUKE3D_COMMON_MIDI_OBJS_EXP:=$(call expandobjs,$(DUKE3D_OBJ)/,$(DUKE3D_COMMON_MIDI_OBJS))

DUKE3D_GAME_SRCS:=$(addprefix $(DUKE3D_SRC)/,$(DUKE3D_GAME_OBJS))
DUKE3D_EDITOR_SRCS:=$(addprefix $(DUKE3D_SRC)/,$(DUKE3D_EDITOR_OBJS))
DUKE3D_GAME_OBJS_EXP:=$(call expandobjs,$(DUKE3D_OBJ)/,$(DUKE3D_GAME_OBJS) $(DUKE3D_GAME_RSRC_OBJS))
DUKE3D_EDITOR_OBJS_EXP:=$(call expandobjs,$(DUKE3D_OBJ)/,$(DUKE3D_EDITOR_OBJS) $(DUKE3D_EDITOR_RSRC_OBJS))

ifneq (0,$(LUNATIC))
    DUKE3D_GAME_SRCS+= $(addprefix $(LUNATIC_SRC)/,$(LUNATIC_GAME_OBJS) $(LUNATIC_LUA_OBJS) $(LUNATIC_GAME_LUA_OBJS))
    DUKE3D_EDITOR_SRCS+= $(addprefix $(LUNATIC_SRC)/,$(LUNATIC_EDITOR_OBJS) $(LUNATIC_LUA_OBJS) $(LUNATIC_EDITOR_LUA_OBJS))
    DUKE3D_GAME_OBJS_EXP+= $(call expandobjs,$(LUNATIC_OBJ)/,$(LUNATIC_GAME_OBJS) $(LUNATIC_LUA_OBJS) $(LUNATIC_GAME_LUA_OBJS))
    DUKE3D_EDITOR_OBJS_EXP+= $(call expandobjs,$(LUNATIC_OBJ)/,$(LUNATIC_EDITOR_OBJS) $(LUNATIC_LUA_OBJS) $(LUNATIC_EDITOR_LUA_OBJS))
endif

DUKE3D_GAME_SRCS_TARGET=$(DUKE3D_GAME_SRCS) $(DUKE3D_COMMON_MIDI_SRCS) $(AUDIOLIB_SRCS) $(MACT_SRCS) $(ENET_SRCS)
DUKE3D_EDITOR_SRCS_TARGET=$(DUKE3D_EDITOR_SRCS) $(AUDIOLIB_SRCS)
DUKE3D_GAME_OBJS_TARGET=$(DUKE3D_GAME_OBJS_EXP) $(DUKE3D_COMMON_MIDI_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(MACT_OBJS_EXP) $(ENET_OBJS_EXP)
DUKE3D_EDITOR_OBJS_TARGET=$(DUKE3D_EDITOR_OBJS_EXP) $(AUDIOLIB_OBJS_EXP)


#### Shadow Warrior

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
    actor.cpp \
    ai.cpp \
    anim.cpp \
    border.cpp \
    break.cpp \
    bunny.cpp \
    cache.cpp \
    cheats.cpp \
    colormap.cpp \
    common.cpp \
    config.cpp \
    console.cpp \
    coolg.cpp \
    coolie.cpp \
    copysect.cpp \
    demo.cpp \
    draw.cpp \
    eel.cpp \
    game.cpp \
    girlninj.cpp \
    goro.cpp \
    grpscan.cpp \
    hornet.cpp \
    interp.cpp \
    interpsh.cpp \
    inv.cpp \
    jplayer.cpp \
    jsector.cpp \
    jweapon.cpp \
    lava.cpp \
    light.cpp \
    mclip.cpp \
    mdastr.cpp \
    menus.cpp \
    miscactr.cpp \
    morph.cpp \
    net.cpp \
    ninja.cpp \
    panel.cpp \
    player.cpp \
    predict.cpp \
    quake.cpp \
    ripper.cpp \
    ripper2.cpp \
    rooms.cpp \
    rotator.cpp \
    rts.cpp \
    save.cpp \
    scrip2.cpp \
    sector.cpp \
    serp.cpp \
    setup.cpp \
    skel.cpp \
    skull.cpp \
    slidor.cpp \
    sounds.cpp \
    spike.cpp \
    sprite.cpp \
    sumo.cpp \
    swconfig.cpp \
    sync.cpp \
    text.cpp \
    track.cpp \
    vator.cpp \
    vis.cpp \
    wallmove.cpp \
    warp.cpp \
    weapon.cpp \
    zilla.cpp \
    zombie.cpp \
    saveable.cpp \

SW_EDITOR_OBJS = \
    jnstub.cpp \
    brooms.cpp \
    bldscript.cpp \
    jbhlp.cpp \
    colormap.cpp \
    grpscan.cpp \
    common.cpp \

SW_GAME_RSRC_OBJS =
SW_EDITOR_RSRC_OBJS =

ifeq (1,$(HAVE_GTK2))
    SW_GAME_OBJS+= startgtk.game.cpp
    SW_GAME_RSRC_OBJS+= game_banner.c
    SW_EDITOR_RSRC_OBJS+= build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    SW_GAME_RSRC_OBJS+= game_icon.c
    SW_EDITOR_RSRC_OBJS+= game_icon.c
endif
ifeq ($(PLATFORM),WINDOWS)
    SW_GAME_OBJS+= startwin.game.cpp
    SW_GAME_RSRC_OBJS+= gameres.rc
    SW_EDITOR_RSRC_OBJS+= buildres.rc
endif

SW_GAME_SRCS:=$(addprefix $(SW_SRC)/,$(SW_GAME_OBJS))
SW_EDITOR_SRCS:=$(addprefix $(SW_SRC)/,$(SW_EDITOR_OBJS))
SW_GAME_OBJS_EXP:=$(call expandobjs,$(SW_OBJ)/,$(SW_GAME_OBJS) $(SW_GAME_RSRC_OBJS))
SW_EDITOR_OBJS_EXP:=$(call expandobjs,$(SW_OBJ)/,$(SW_EDITOR_OBJS) $(SW_EDITOR_RSRC_OBJS))

SW_GAME_SRCS_TARGET=$(SW_GAME_SRCS) $(DUKE3D_COMMON_MIDI_SRCS) $(AUDIOLIB_SRCS) $(MACT_SRCS)
SW_EDITOR_SRCS_TARGET=$(SW_EDITOR_SRCS) $(AUDIOLIB_SRCS)
SW_GAME_OBJS_TARGET=$(SW_GAME_OBJS_EXP) $(DUKE3D_COMMON_MIDI_OBJS_EXP) $(AUDIOLIB_OBJS_EXP) $(MACT_OBJS_EXP)
SW_EDITOR_OBJS_TARGET=$(SW_EDITOR_OBJS_EXP) $(AUDIOLIB_OBJS_EXP)


##### Recipes

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
    start \
    $(foreach j,$(foreach i,$(GAMES),$($i)) test utils tools,$j clean$j) \
    veryclean \
    clean \
    printutils \
    printtools \
    rev \
    $(ENGINE_OBJ)/rev.$o \
    clang-tools \

.SUFFIXES:
.SECONDEXPANSION:


#### Targets

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

$$($1_$2)$$(EXESUFFIX): $$($1_$2_OBJS_TARGET) $$(DUKE3D_COMMON_$2_OBJS_TARGET) $$(ENGINE_OBJS_EXP) $$($1_$2_MISCDEPS) | $$($1_$2_ORDERONLYDEPS)
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


#### Rules

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


### Lunatic

# Create object files directly with luajit
$(DUKE3D_OBJ)/%.$o: $(LUNATIC_SRC)/%.lua | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(LUAJIT) -bg $(LUAJIT_BCOPTS) $< $@ $(RECIPE_RESULT_COMPILE)

$(DUKE3D_OBJ)/%.$o: $(LUNATIC_SRC)/%.cpp | $(DUKE3D_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(DUKE3D_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

# List of exported symbols, OS X
$(DUKE3D_OBJ)/lunatic_%_osx: $(LUNATIC_SRC)/%.lds | $(DUKE3D_OBJ)
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

# List of exported symbols, Windows
$(DUKE3D_OBJ)/lunatic_%.def: $(LUNATIC_SRC)/%.lds | $(DUKE3D_OBJ)
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@


### Main Rules

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

## Cosmetic stuff

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


### Other special cases

# Comment out the following rule to debug a-c.o
$(ENGINE_OBJ)/a-c.$o: $(ENGINE_SRC)/a-c.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(subst -O$(OPTLEVEL),-O2,$(subst $(CLANG_DEBUG_FLAGS),,$(COMPILER_CXX))) $(ENGINE_CFLAGS) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(ENGINE_OBJ)/rev.$o: $(ENGINE_SRC)/rev.cpp | $(ENGINE_OBJ)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(ENGINE_CFLAGS) $(REVFLAG) -c $< -o $@ $(RECIPE_RESULT_COMPILE)


### Directories

$(obj):
	-mkdir $@ $(DONT_PRINT) $(DONT_FAIL)

$(foreach i,$(COMPONENTS),$($i_OBJ)): | $(obj)
	-mkdir $@ $(DONT_PRINT) $(DONT_FAIL)


### Phonies

clang-tools: $(filter %.c %.cpp %.m %.mm,$(DUKE3D_GAME_SRCS_TARGET) $(DUKE3D_EDITOR_SRCS_TARGET) $(DUKE3D_COMMON_EDITOR_SRCS_TARGET) $(ENGINE_SRCS))
	echo $^ -- $(COMPILERFLAGS) $(foreach i,$(COMPONENTS),$($i_CFLAGS)) $(CWARNS)

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


### Compatibility

test: kenbuild
utils: tools
printutils: printtools
veryclean: clean
cleanutils: cleantools
cleantest: cleankenbuild
