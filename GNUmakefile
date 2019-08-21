#
# EDuke32 Makefile for GNU Make
#

### Global Profiles
ifeq ($(FURY),1)
    APPNAME := Ion Fury
    APPBASENAME := fury
    STANDALONE := 1
    POLYMER := 0
    USE_LIBVPX := 0
    NETCODE := 0
    SIMPLE_MENU := 1
endif

### Platform and Toolchain Configuration
include Common.mak

### File Extensions
asm := nasm
o := o

### Directories
source := source
obj := obj

### Functions
define parent
$(word 1,$(subst _, ,$1))
endef
define expandobjs
$$(addprefix $$($$(call parent,$1)_obj)/,$$(addsuffix .$$o,$$(basename $$($1_objs) $$($1_rsrc_objs) $$($1_gen_objs))))
endef
define expandsrcs
$(addprefix $($(call parent,$1)_src)/,$($1_objs)) $(addprefix $($(call parent,$1)_rsrc)/,$($1_rsrc_objs)) $(addprefix $($(call parent,$1)_obj)/,$($1_gen_objs))
endef
define expanddeps
$(strip $1 $(foreach j,$1,$(call $0,$($j_deps))))
endef
define getdeps
$(call expanddeps,$1_$2 $(common_$2_deps) engine)
endef


##### External Library Definitions

#### libxmp-lite

libxmplite := libxmp-lite

libxmplite_objs := \
    control.c \
    dataio.c \
    effects.c \
    filter.c \
    format.c \
    hio.c \
    lfo.c \
    load.c \
    load_helpers.c \
    memio.c \
    mixer.c \
    mix_all.c \
    period.c \
    player.c \
    read_event.c \
    scan.c \
    smix.c \
    virtual.c \
    common.c \
    itsex.c \
    it_load.c \
    mod_load.c \
    mtm_load.c \
    s3m_load.c \
    sample.c \
    xm_load.c \

libxmplite_root := $(source)/$(libxmplite)
libxmplite_src := $(libxmplite_root)/src
libxmplite_inc := $(libxmplite_root)/include
libxmplite_obj := $(obj)/$(libxmplite)

libxmplite_cflags := -DHAVE_ROUND -DLIBXMP_CORE_PLAYER -DBUILDING_STATIC -I$(libxmplite_inc)/libxmp-lite -Wno-unused-parameter -Wno-sign-compare


#### LPeg

lpeg := lpeg

lpeg_objs := \
    lpcap.c \
    lpcode.c \
    lpprint.c \
    lptree.c \
    lpvm.c \

lpeg_root := $(source)/$(lpeg)
lpeg_src := $(lpeg_root)/src
lpeg_inc := $(lpeg_root)/include
lpeg_obj := $(obj)/$(lpeg)


#### PhysicsFS

physfs := physfs

physfs_objs := \
    physfs.c \
    physfs_archiver_7z.c \
    physfs_archiver_dir.c \
    physfs_archiver_grp.c \
    physfs_archiver_hog.c \
    physfs_archiver_iso9660.c \
    physfs_archiver_mvl.c \
    physfs_archiver_qpak.c \
    physfs_archiver_slb.c \
    physfs_archiver_unpacked.c \
    physfs_archiver_vdf.c \
    physfs_archiver_wad.c \
    physfs_archiver_zip.c \
    physfs_byteorder.c \
    physfs_unicode.c \

ifeq ($(PLATFORM),APPLE)
    physfs_objs += physfs_platform_apple.m
else ifeq ($(PLATFORM),WINDOWS)
    physfs_objs += physfs_platform_windows.c
else
    physfs_objs += physfs_platform_unix.c
endif

physfs_root := $(source)/$(physfs)
physfs_src := $(physfs_root)/src
physfs_inc := $(physfs_root)/include
physfs_obj := $(obj)/$(physfs)

physfs_cflags :=


#### ENet

enet := enet

enet_objs := \
    callbacks.c \
    host.c \
    list.c \
    packet.c \
    peer.c \
    protocol.c \
    compress.c \

enet_root := $(source)/$(enet)
enet_src := $(enet_root)/src
enet_inc := $(enet_root)/include
enet_obj := $(obj)/$(enet)

enet_cflags :=

ifeq ($(PLATFORM),WINDOWS)
    enet_objs += win32.c
else
    enet_objs += unix.c
    enet_cflags += -DHAS_SOCKLEN_T
endif


#### glad

glad := glad

glad_objs := \
    glad.c \

glad_root := $(source)/$(glad)
glad_src := $(glad_root)/src
glad_inc := $(glad_root)/include
glad_obj := $(obj)/$(glad)

glad_cflags :=

ifeq ($(RENDERTYPE),WIN)
    glad_objs += glad_wgl.c
endif


##### Component Definitions

#### EBacktrace

ifndef ebacktrace_dll
    ebacktrace_dll := ebacktrace1.dll
    ifeq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        ebacktrace_dll := ebacktrace1-64.dll
    endif
endif


#### BUILD Engine

engine := build

engine_root := $(source)/$(engine)
engine_src := $(engine_root)/src
engine_inc := $(engine_root)/include
engine_obj := $(obj)/$(engine)

engine_cflags := -I$(engine_src)

engine_deps :=

ifneq (0,$(USE_PHYSFS))
    engine_deps += physfs
endif

engine_objs := \
    rev.cpp \
    baselayer.cpp \
    vfs.cpp \
    cache1d.cpp \
    klzw.cpp \
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
    polymost1Frag.glsl \
    polymost1Vert.glsl \
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
    softsurface.cpp \
    mmulti_null.cpp \
    mutex.cpp \
    timer.cpp \
    xxhash.c \
    md4.cpp \
    colmatch.cpp \
    screenshot.cpp \
    mhk.cpp \
    pngwrite.cpp \
    miniz.c \
    miniz_tinfl.c \
    miniz_tdef.c \
    fix16.cpp \
    fix16_str.cpp \

engine_editor_objs := \
    build.cpp \
    config.cpp \
    defs.cpp \

engine_tools_objs := \
    compat.cpp \
    pragmas.cpp \
    kplib.cpp \
    cache1d.cpp \
    klzw.cpp \
    crc32.cpp \
    colmatch.cpp \
    lz4.cpp \

ifeq (0,$(NOASM))
  engine_objs += a.nasm
else
  engine_objs += a-c.cpp
  ifneq (0,$(USE_ASM64))
    engine_objs += a64.yasm
  endif
endif
ifeq (1,$(USE_OPENGL))
    engine_objs += glsurface.cpp voxmodel.cpp mdsprite.cpp tilepacker.cpp
    engine_deps += glad
    ifeq (1,$(POLYMER))
        engine_objs += glbuild.cpp polymer.cpp
    endif
endif
ifneq (0,$(LUNATIC))
    engine_objs += lunatic.cpp
endif
ifeq ($(PLATFORM),DARWIN)
    engine_objs += osxbits.mm
    engine_tools_objs += osxbits.mm
    ifeq ($(STARTUP_WINDOW),1)
        engine_editor_objs += startosx.editor.mm
    endif
    ifeq ($(SDL_TARGET),1)
        ifneq ($(SDL_FRAMEWORK),0)
            engine_objs += SDLMain.mm
        endif
    endif
endif
ifeq ($(PLATFORM),WINDOWS)
    engine_objs += winbits.cpp
    ifeq ($(STARTUP_WINDOW),1)
        engine_editor_objs += startwin.editor.cpp
    endif
endif
ifeq ($(PLATFORM),WII)
    engine_objs += wiibits.cpp
    LINKERFLAGS += -Wl,-wrap,c_default_exceptionhandler
endif
ifeq ($(RENDERTYPE),SDL)
    engine_objs += sdlayer.cpp

    ifeq (1,$(HAVE_GTK2))
        engine_objs += gtkbits.cpp dynamicgtk.cpp
        ifeq ($(STARTUP_WINDOW),1)
            engine_editor_objs += startgtk.editor.cpp
        endif
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    engine_objs += winlayer.cpp rawinput.cpp
endif

ifneq ($(USE_LIBVPX),0)
    engine_objs += animvpx.cpp
endif


#### mact

mact := mact

mact_root := $(source)/$(mact)
mact_src := $(mact_root)/src
mact_inc := $(mact_root)/include
mact_obj := $(obj)/$(mact)

mact_objs := \
    control.cpp \
    keyboard.cpp \
    joystick.cpp \
    scriplib.cpp \
    animlib.cpp \


#### AudioLib

audiolib := audiolib

audiolib_objs := \
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

audiolib_root := $(source)/$(audiolib)
audiolib_src := $(audiolib_root)/src
audiolib_inc := $(audiolib_root)/include
audiolib_obj := $(obj)/$(audiolib)

audiolib_cflags :=

audiolib_deps :=

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(MIXERTYPE),WIN)
        audiolib_objs += driver_directsound.cpp
    endif
endif

ifeq ($(MIXERTYPE),SDL)
    ifeq (,$(filter $(PLATFORM),DARWIN WINDOWS WII))
        audiolib_cflags += `$(PKG_CONFIG) --cflags vorbis`
    endif
    audiolib_objs += driver_sdl.cpp
endif

ifneq (0,$(HAVE_XMP))
    audiolib_cflags += -I$(libxmplite_inc)
    audiolib_deps += libxmplite
endif


#### Tools

tools := tools

tools_objs := \
    compat_tools.cpp \

tools_root := $(source)/$(tools)
tools_src := $(tools_root)/src
tools_obj := $(obj)/$(tools)

tools_cflags := $(engine_cflags)

tools_deps := engine_tools

tools_targets := \
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
    tools_targets += enumdisplay getdxdidf
endif
ifeq ($(RENDERTYPE),SDL)
    tools_targets += makesdlkeytrans
endif


#### KenBuild (Test Game)

kenbuild := kenbuild

kenbuild_root := $(source)/$(kenbuild)
kenbuild_src := $(kenbuild_root)/src
kenbuild_rsrc := $(kenbuild_root)/rsrc
kenbuild_obj := $(obj)/$(kenbuild)

kenbuild_cflags := -I$(kenbuild_src)

kenbuild_game := ekenbuild
kenbuild_editor := ekenbuild-editor

kenbuild_game_proper := EKenBuild
kenbuild_editor_proper := EKenBuild Editor

kenbuild_game_objs := \
    game.cpp \
    sound_stub.cpp \
    common.cpp \
    config.cpp \

kenbuild_editor_objs := \
    bstub.cpp \
    common.cpp \

kenbuild_game_rsrc_objs :=
kenbuild_editor_rsrc_objs :=
kenbuild_game_gen_objs :=
kenbuild_editor_rsrc_objs :=

ifeq (1,$(HAVE_GTK2))
    kenbuild_game_objs += startgtk.game.cpp
    kenbuild_game_gen_objs += game_banner.c
    kenbuild_editor_gen_objs += build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    kenbuild_game_rsrc_objs += game_icon.c
    kenbuild_editor_rsrc_objs += build_icon.c
endif
ifeq ($(PLATFORM),WINDOWS)
    kenbuild_game_objs += startwin.game.cpp
    kenbuild_game_rsrc_objs += gameres.rc
    kenbuild_editor_rsrc_objs += buildres.rc
endif

ifeq ($(PLATFORM),DARWIN)
    ifeq ($(STARTUP_WINDOW),1)
        kenbuild_game_objs += StartupWinController.game.mm
    endif
endif


#### Duke Nukem 3D

duke3d := duke3d

duke3d_game_ldflags :=
duke3d_editor_ldflags :=

duke3d_game_stripflags :=
duke3d_editor_stripflags :=

duke3d_root := $(source)/$(duke3d)
duke3d_src := $(duke3d_root)/src
duke3d_rsrc := $(duke3d_root)/rsrc
duke3d_obj := $(obj)/$(duke3d)

ifeq ($(FURY),1)
    ifeq ($(PLATFORM),WINDOWS)
        duke3d_rsrc := $(duke3d_root)/rsrc/fury
    endif
    duke3d_obj := $(obj)/fury
endif

duke3d_cflags := -I$(duke3d_src)

common_editor_deps := duke3d_common_editor engine_editor

duke3d_game_deps := duke3d_common_midi audiolib mact
duke3d_editor_deps := audiolib

ifneq (0,$(NETCODE))
    duke3d_game_deps += enet
endif

ifneq (0,$(LUNATIC))
    duke3d_game_deps += lunatic lunatic_game lpeg
    duke3d_editor_deps += lunatic lunatic_editor lpeg
endif

duke3d_game := eduke32
duke3d_editor := mapster32

ifneq (,$(APPBASENAME))
    duke3d_game := $(APPBASENAME)
endif

duke3d_game_proper := EDuke32
duke3d_editor_proper := Mapster32

duke3d_common_editor_objs := \
    m32common.cpp \
    m32def.cpp \
    m32exec.cpp \
    m32vars.cpp \

duke3d_game_objs := \
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
    common.cpp \
    config.cpp \
    demo.cpp \
    input.cpp \
    menus.cpp \
    namesdyn.cpp \
    network.cpp \
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

duke3d_editor_objs := \
    astub.cpp \
    common.cpp \
    grpscan.cpp \
    sounds_mapster32.cpp \

duke3d_game_rsrc_objs :=
duke3d_editor_rsrc_objs :=
duke3d_game_gen_objs :=
duke3d_editor_gen_objs :=

duke3d_game_miscdeps :=
duke3d_editor_miscdeps :=
duke3d_game_orderonlydeps :=
duke3d_editor_orderonlydeps :=

## Lunatic devel
lunatic := lunatic
lunatic_src := $(duke3d_src)/$(lunatic)
lunatic_obj := $(duke3d_obj)

ifneq (0,$(LUNATIC))
    COMPILERFLAGS += -I$(lunatic_src) -DLUNATIC

    # Determine size of _defs*.lua bytecode once.
    ifndef DEFS_BC_SIZE
        DEFS_BC_SIZE := $(shell $(LUAJIT) -bg -t h $(lunatic_src)/_defs_game.lua -)
        DEFS_BC_SIZE := $(word 3, $(DEFS_BC_SIZE))
    endif
    ifndef DEFS_M32_BC_SIZE
        DEFS_M32_BC_SIZE := $(shell $(LUAJIT) -bg -t h $(lunatic_src)/_defs_editor.lua -)
        DEFS_M32_BC_SIZE := $(word 3, $(DEFS_M32_BC_SIZE))
    endif
    duke3d_cflags += -DLUNATIC_DEFS_BC_SIZE=$(DEFS_BC_SIZE) -DLUNATIC_DEFS_M32_BC_SIZE=$(DEFS_M32_BC_SIZE)

    # Lunatic object base names. These are not used in targets directly.
    lunatic_objs := \
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

    lunatic_game_objs := \
        lunatic_game.cpp \
        _defs_game.lua \
        con_lang.lua \
        lunacon.lua \
        randgen.lua \
        stat.lua \
        control.lua \
        lunasave.lua \
        fs.lua \

    lunatic_editor_objs := \
        lunatic_editor.cpp \
        _defs_editor.lua \

    # TODO: remove debugging modules from release build

    # now, take care of having the necessary symbols (sector, wall, etc.) in the
    # executable no matter what the debugging level

    ifeq ($(PLATFORM),DARWIN)
        # strip on OSX says: removing global symbols from a final linked no longer supported.
        #                    Use -exported_symbols_list at link time when building
        # But, following _their_ directions does not give us the symbols! wtf?
        # Instead of using -alias_list and -exported_symbols_list, prevent stripping them.
        duke3d_game_stripflags += -s $(duke3d_obj)/lunatic_dynsymlist_game_osx
        duke3d_editor_stripflags += -s $(duke3d_obj)/lunatic_dynsymlist_editor_osx

        duke3d_game_orderonlydeps += $(duke3d_obj)/lunatic_dynsymlist_game_osx
        duke3d_editor_orderonlydeps += $(duke3d_obj)/lunatic_dynsymlist_editor_osx
        LINKERFLAGS += -pagezero_size 10000 -image_base 100000000
    endif
    ifeq ($(PLATFORM),WINDOWS)
        override STRIP :=
        duke3d_game_miscdeps += $(duke3d_obj)/lunatic_dynsymlist_game.def
        duke3d_editor_miscdeps += $(duke3d_obj)/lunatic_dynsymlist_editor.def
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        override STRIP :=
        duke3d_game_ldflags += -Wl,--dynamic-list=$(lunatic_src)/dynsymlist_game.lds
        duke3d_editor_ldflags += -Wl,--dynamic-list=$(lunatic_src)/dynsymlist_editor.lds
    endif
endif

ifeq ($(SUBPLATFORM),LINUX)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),BSD)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lexecinfo
endif

ifeq ($(PLATFORM),DARWIN)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm \
            -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
            -Wl,-framework,CoreMIDI -Wl,-framework,AudioUnit \
            -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL
    ifneq (00,$(DARWIN9)$(DARWIN10))
        LIBS += -Wl,-framework,QuickTime -lm
    endif

    ifeq ($(STARTUP_WINDOW),1)
        duke3d_game_objs += GrpFile.game.mm GameListSource.game.mm startosx.game.mm
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
    duke3d_game_objs += winbits.cpp
    duke3d_game_rsrc_objs += gameres.rc
    duke3d_editor_rsrc_objs += buildres.rc
    ifeq ($(STARTUP_WINDOW),1)
        duke3d_game_objs += startwin.game.cpp
    endif
    ifeq ($(MIXERTYPE),WIN)
        LIBS += -ldsound
        duke3d_common_midi_objs := music.cpp midi.cpp mpu401.cpp
    endif
endif

ifeq ($(PLATFORM),WII)
    LIBS += -lvorbisidec
endif

ifeq (11,$(HAVE_GTK2)$(STARTUP_WINDOW))
    duke3d_game_objs += startgtk.game.cpp
    duke3d_game_gen_objs += game_banner.c
    duke3d_editor_gen_objs += build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    duke3d_game_rsrc_objs += game_icon.c
    duke3d_editor_rsrc_objs += build_icon.c
endif
ifeq ($(MIXERTYPE),SDL)
    duke3d_common_midi_objs := sdlmusic.cpp
endif


#### Shadow Warrior

sw := sw

sw_root := $(source)/$(sw)
sw_src := $(sw_root)/src
sw_rsrc := $(sw_root)/rsrc
sw_obj := $(obj)/$(sw)

sw_cflags := -I$(sw_src)

sw_game_deps := duke3d_common_midi audiolib mact
sw_editor_deps := audiolib

sw_game := voidsw
sw_editor := wangulator

sw_game_proper := VoidSW
sw_editor_proper := Wangulator

sw_game_objs := \
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
    network.cpp \
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

sw_editor_objs := \
    jnstub.cpp \
    brooms.cpp \
    bldscript.cpp \
    jbhlp.cpp \
    colormap.cpp \
    grpscan.cpp \
    common.cpp \

sw_game_rsrc_objs :=
sw_editor_rsrc_objs :=
sw_game_gen_objs :=
sw_editor_gen_objs :=

ifeq (1,$(HAVE_GTK2))
    sw_game_objs += startgtk.game.cpp
    sw_game_gen_objs += game_banner.c
    sw_editor_gen_objs += build_banner.c
endif
ifeq ($(RENDERTYPE),SDL)
    sw_game_rsrc_objs += game_icon.c
    sw_editor_rsrc_objs += game_icon.c
endif
ifeq ($(PLATFORM),WINDOWS)
    sw_game_objs += startwin.game.cpp
    sw_game_rsrc_objs += gameres.rc
    sw_editor_rsrc_objs += buildres.rc
endif


#### Final setup

COMPILERFLAGS += -I$(engine_inc) -I$(mact_inc) -I$(audiolib_inc) -I$(enet_inc) -I$(glad_inc) -MP -MMD
ifneq (0,$(USE_PHYSFS))
    COMPILERFLAGS += -I$(physfs_inc) -DUSE_PHYSFS
endif


##### Recipes

games := \
    kenbuild \
    duke3d \
    sw \

libraries := \
    engine \
    audiolib \
    mact \
    enet \
    libxmplite \
    lpeg \
    glad \

ifneq (0,$(USE_PHYSFS))
    libraries += physfs
endif

components := \
    $(games) \
    $(libraries) \
    tools \

roles := \
    game \
    editor \


ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: \
    all \
    start \
    $(addprefix clean,$(games) test utils tools) \
    veryclean \
    clean \
    printutils \
    printtools \
    rev \
    $(engine_obj)/rev.$o \
    clang-tools \

.SUFFIXES:
.SECONDEXPANSION:


#### Targets

all: duke3d

start:
	$(BUILD_STARTED)

tools: $(addsuffix $(EXESUFFIX),$(tools_targets)) | start
	@$(call LL,$^)

$(games): $$(foreach i,$(roles),$$($$@_$$i)$(EXESUFFIX)) | start
	@$(call LL,$^)

ebacktrace: $(ebacktrace_dll) | start
	@$(call LL,$^)

ifeq ($(PLATFORM),WII)
ifneq ($(ELF2DOL),)
%$(DOLSUFFIX): %$(EXESUFFIX)
endif
endif


define BUILDRULE

$$($1_$2)$$(EXESUFFIX): $$(foreach i,$(call getdeps,$1,$2),$$(call expandobjs,$$i)) $$($1_$2_miscdeps) | $$($1_$2_orderonlydeps)
	$$(LINK_STATUS)
	$$(RECIPE_IF) $$(LINKER) -o $$@ $$^ $$(GUI_LIBS) $$($1_$2_ldflags) $$(LIBDIRS) $$(LIBS) $$(RECIPE_RESULT_LINK)
ifeq ($$(PLATFORM),WII)
ifneq ($$(ELF2DOL),)
	$$(ELF2DOL) $$@ $$($1_$2)$$(DOLSUFFIX)
endif
endif
ifneq ($$(STRIP),)
	$$(STRIP) $$@ $$($1_$2_stripflags)
endif
ifeq ($$(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/$$($1_$2_proper).app" "./"
	$(call MKDIR,"$$($1_$2_proper).app/Contents/MacOS")
	cp -f "$$($1_$2)$$(EXESUFFIX)" "$$($1_$2_proper).app/Contents/MacOS/"
endif

endef

$(foreach i,$(games),$(foreach j,$(roles),$(eval $(call BUILDRULE,$i,$j))))


#### Rules

$(ebacktrace_dll): platform/Windows/src/backtrace.c
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(CC) $(CONLYFLAGS) -O2 -ggdb -shared -Wall -Wextra -static-libgcc -I$(engine_inc) -o $@ $^ -lbfd -liberty -limagehlp $(RECIPE_RESULT_COMPILE)

libklzw$(DLLSUFFIX): $(engine_src)/klzw.cpp
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_C) -shared -fPIC $< -o $@ $(RECIPE_RESULT_COMPILE)

# to debug the tools link phase, make a copy of this rule explicitly replacing % with the name of a tool, such as kextract
%$(EXESUFFIX): $(tools_obj)/%.$o $(foreach i,tools $(tools_deps),$(call expandobjs,$i))
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) $(RECIPE_RESULT_LINK)

enumdisplay$(EXESUFFIX): $(tools_obj)/enumdisplay.$o
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) -lgdi32 $(RECIPE_RESULT_LINK)
getdxdidf$(EXESUFFIX): $(tools_obj)/getdxdidf.$o
	$(LINK_STATUS)
	$(RECIPE_IF) $(LINKER) -o $@ $^ $(LIBDIRS) $(LIBS) -ldinput $(RECIPE_RESULT_LINK)


### Lunatic

# Create object files directly with luajit
$(duke3d_obj)/%.$o: $(lunatic_src)/%.lua | $(duke3d_obj)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(LUAJIT) -bg $(LUAJIT_BCOPTS) $< $@ $(RECIPE_RESULT_COMPILE)

$(duke3d_obj)/%.$o: $(lunatic_src)/%.cpp | $(duke3d_obj)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(duke3d_cflags) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

# List of exported symbols, OS X
$(duke3d_obj)/lunatic_%_osx: $(lunatic_src)/%.lds | $(duke3d_obj)
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

# List of exported symbols, Windows
$(duke3d_obj)/lunatic_%.def: $(lunatic_src)/%.lds | $(duke3d_obj)
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@


### Main Rules

define OBJECTRULES

include $(wildcard $($1_obj)/*.d)

$$($1_obj)/%.$$o: $$($1_src)/%.nasm | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(AS) $$(ASFLAGS) $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.yasm | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(AS) $$(ASFLAGS) $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.c | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.cpp | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_CXX) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.m | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_OBJC) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.mm | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_OBJCXX) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_obj)/%.c | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_src)/%.glsl | $$($1_obj)
	@echo Creating $$($1_obj)/$$(<F).cpp from $$<
	@$$(call RAW_ECHO,extern char const *$$(basename $$(<F));) > $$($1_obj)/$$(<F).cpp
	@$$(call RAW_ECHO,char const *$$(basename $$(<F)) = R"shader$$(paren_open)) >> $$($1_obj)/$$(<F).cpp
	@$$(call CAT,$$<) >> $$($1_obj)/$$(<F).cpp
	@$$(call RAW_ECHO,$$(paren_close)shader";) >> $$($1_obj)/$$(<F).cpp
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_CXX) $$($1_cflags) -c $$($1_obj)/$$(<F).cpp -o $$@ $$(RECIPE_RESULT_COMPILE)

## Cosmetic stuff

$$($1_obj)/%.$$o: $$($1_rsrc)/%.rc | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(RC) -i $$< -o $$@ --include-dir=$$(engine_inc) --include-dir=$$($1_src) --include-dir=$$($1_rsrc) -DPOLYMER=$$(POLYMER) $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%.$$o: $$($1_rsrc)/%.c | $$($1_obj)
	$$(COMPILE_STATUS)
	$$(RECIPE_IF) $$(COMPILER_C) $$($1_cflags) -c $$< -o $$@ $$(RECIPE_RESULT_COMPILE)

$$($1_obj)/%_banner.c: $$($1_rsrc)/%.bmp | $$($1_obj)
	echo "#include \"gtkpixdata_shim.h\"" > $$@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $$^ | sed 's/load_inc//' >> $$@

endef

$(foreach i,$(components),$(eval $(call OBJECTRULES,$i)))


### Other special cases

# Comment out the following rule to debug a-c.o
$(engine_obj)/a-c.$o: $(engine_src)/a-c.cpp | $(engine_obj)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(subst -O$(OPTLEVEL),-O2,$(subst $(ASAN_FLAGS),,$(COMPILER_CXX))) $(engine_cflags) -c $< -o $@ $(RECIPE_RESULT_COMPILE)

$(engine_obj)/rev.$o: $(engine_src)/rev.cpp | $(engine_obj)
	$(COMPILE_STATUS)
	$(RECIPE_IF) $(COMPILER_CXX) $(engine_cflags) $(REVFLAG) -c $< -o $@ $(RECIPE_RESULT_COMPILE)


### Directories

$(foreach i,$(components),$($i_obj)):
	-$(call MKDIR,$@)

### Phonies

clang-tools: $(filter %.c %.cpp,$(foreach i,$(call getdeps,duke3d,game),$(call expandsrcs,$i)))
	echo $^ -- -x c++ $(CXXONLYFLAGS) $(COMPILERFLAGS) $(CWARNS) $(foreach i,$(components),$($i_cflags))

$(addprefix clean,$(games)):
	-$(call RM,$(foreach i,$(roles),$($(subst clean,,$@)_$i)$(EXESUFFIX)))
	-$(call RMDIR,$($(subst clean,,$@)_obj))
ifeq ($(PLATFORM),DARWIN)
	-$(call RMDIR,$(foreach i,$(roles),"$($(subst clean,,$@)_$i_proper).app"))
endif

cleantools:
	-$(call RM,$(addsuffix $(EXESUFFIX),$($(subst clean,,$@)_targets)))
	-$(call RMDIR,$($(subst clean,,$@)_obj))

clean: cleanduke3d cleantools
	-$(call RMDIR,$(obj))
	-$(call RM,$(ebacktrace_dll))

printtools:
	echo "$(addsuffix $(EXESUFFIX),$(tools_targets))"

rev: $(engine_obj)/rev.$o


### Compatibility

test: kenbuild
utils: tools
printutils: printtools
veryclean: clean
cleanutils: cleantools
cleantest: cleankenbuild
