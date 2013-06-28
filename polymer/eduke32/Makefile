#
# EDuke32 Makefile for GNU Make
#

include Makefile.common


# Build locations
#
SRC=source
RSRC=rsrc
ESRC=$(EROOT)/src
EINC=$(EROOT)/include
INC=$(SRC)
o=o
# ENETROOT=$(ESRC)/enet

ifneq (0,$(RELEASE))
    # Debugging disabled
    debug+= $(F_NO_STACK_PROTECTOR)
else
    # Debugging enabled
    ifneq (0,$(KRANDDEBUG))
        debug+= -fno-inline -fno-inline-functions -fno-inline-functions-called-once
    endif
endif

OURCOMMONFLAGS=$(BASECOMMONFLAGS) \
    -I$(INC) -I$(EINC) -I$(SRC)/jmact -I$(JAUDIOLIBDIR)/include -I$(ENETDIR)/include 
OURCFLAGS=$(OURCOMMONFLAGS) $(BASECFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURASFLAGS=$(BASEASFLAGS)
PRINTLDFLAGS=$(BASELDFLAGS)
OURLDFLAGS=$(OURCOMMONFLAGS) $(PRINTLDFLAGS)

# Game/editor-specific linker options
GAMELDFLAGS=
EDITORLDFLAGS=

LIBS=
LIBDIRS=

JAUDIOLIBDIR=$(SRC)/jaudiolib
JAUDIOLIB=libjfaudiolib.a

ENETDIR=$(SRC)/enet
ENETLIB=libenet.a

ifeq ($(NETCODE),0)
    ENET_TARGET=
else
    ENET_TARGET=$(ENETDIR)/$(ENETLIB)
endif


include $(EROOT)/Makefile.shared

# The reasoning for this order is so SDL_mixer can link to SDL, etc.
OURLIBS=$(LIBDIRS) $(BASELIBDIRS) $(BUILDLIBDIRS) $(LIBS) $(BASELIBS) $(BUILDLIBS)


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
    OBJ=$(SRC)/obj_win
    EOBJ=$(SRC)/eobj_win
else
    ifeq ($(SUBPLATFORM),LINUX)
        LIBS+= -lrt
    endif
    OBJ=$(SRC)/obj
    EOBJ=$(SRC)/eobj
endif

JMACTOBJ=$(OBJ)/file_lib.$o \
	$(OBJ)/control.$o \
	$(OBJ)/keyboard.$o \
	$(OBJ)/mouse.$o \
	$(OBJ)/joystick.$o \
	$(OBJ)/mathutil.$o \
	$(OBJ)/scriplib.$o \
	$(OBJ)/animlib.$o

GAMEOBJS=$(OBJ)/game.$o \
	$(OBJ)/actors.$o \
	$(OBJ)/anim.$o \
	$(OBJ)/common.$o \
	$(OBJ)/config.$o \
	$(OBJ)/demo.$o \
	$(OBJ)/gamedef.$o \
	$(OBJ)/gameexec.$o \
	$(OBJ)/gamevars.$o \
	$(OBJ)/global.$o \
	$(OBJ)/input.$o \
	$(OBJ)/menus.$o \
	$(OBJ)/namesdyn.$o \
	$(OBJ)/net.$o \
	$(OBJ)/player.$o \
	$(OBJ)/premap.$o \
	$(OBJ)/savegame.$o \
	$(OBJ)/sector.$o \
	$(OBJ)/rts.$o \
	$(OBJ)/osdfuncs.$o \
	$(OBJ)/osdcmds.$o \
	$(OBJ)/grpscan.$o \
	$(OBJ)/sounds.$o \
	$(OBJ)/soundsdyn.$o \
	$(JMACTOBJ)

EDITOROBJS=$(OBJ)/astub.$o \
	$(OBJ)/common.$o \
	$(OBJ)/m32def.$o \
	$(OBJ)/m32exec.$o \
	$(OBJ)/m32vars.$o \
	$(OBJ)/mathutil.$o \
	$(OBJ)/sounds_mapster32.$o

ifneq ($(USE_LIBVPX),0)
    GAMEOBJS+= $(OBJ)/animvpx.$o
endif

ifneq (0,$(DISABLEINLINING))
    GAMEOBJS+= $(OBJ)/game_inline.$o \
	$(OBJ)/actors_inline.$o \
	$(OBJ)/sector_inline.$o
endif

MISCGAMEDEPS=
MISCEDITORDEPS=


## Lunatic devel
ifneq (0,$(LUNATIC))
    LUNATIC_COMMON_OBJS = $(OBJ)/luaJIT_BC_defs_common.$o $(OBJ)/luaJIT_BC_engine_maptext.$o
    EDITOROBJS+= $(OBJ)/lunatic_m32.$o $(LUNATIC_COMMON_OBJS)
    GAMEOBJS+= $(OBJ)/lunatic_game.$o $(LUNATIC_COMMON_OBJS)

    GAMEOBJS+= $(OBJ)/../lpeg.a  # TEMP
    GAMEOBJS+= $(OBJ)/luaJIT_BC_con_lang.$o \
               $(OBJ)/luaJIT_BC_lunacon.$o \
               $(OBJ)/luaJIT_BC_randgen.$o \
               $(OBJ)/luaJIT_BC_stat.$o \
               $(OBJ)/luaJIT_BC_bitar.$o \
               $(OBJ)/luaJIT_BC_control.$o \
               $(OBJ)/luaJIT_BC_bcarray.$o \
               $(OBJ)/luaJIT_BC_bcheck.$o \
               $(OBJ)/luaJIT_BC_xmath.$o \
               $(OBJ)/luaJIT_BC_defs.$o \
               $(OBJ)/luaJIT_BC_v.$o \
               $(OBJ)/luaJIT_BC_dump.$o \
               $(OBJ)/luaJIT_BC_dis_x86.$o \
               $(OBJ)/luaJIT_BC_dis_x64.$o \
               $(OBJ)/luaJIT_BC_savegame.$o \
    # TODO: remove debugging modules from release build

    # now, take care of having the necessary symbols (sector, wall, etc.) in the
    # executable no matter what the debugging level

    ifeq ($(PLATFORM),DARWIN)
        # strip on OSX says: removing global symbols from a final linked no longer supported.
        #                    Use -exported_symbols_list at link time when building
        # But, following _their_ directions does not give us the symbols! wtf?
        ifneq ($(STRIP),0)
            STRIP+= -s $(SRC)/lunatic/dynsymlist_osx
        endif

        MISCGAMEDEPS+= $(SRC)/lunatic/dynsymlist_osx
        PRINTLDFLAGS+= -pagezero_size 10000 -image_base 100000000 #-Wl,-alias_list -Wl,$(SRC)/lunatic/aliases_list #-exported_symbols_list $(SRC)/lunatic/dynsymlist_osx
    endif
    ifeq ($(PLATFORM),WINDOWS)
        override STRIP=
        MISCGAMEDEPS+= $(SRC)/lunatic/eduke32.def
        GAMELDFLAGS+= $(SRC)/lunatic/eduke32.def
        MISCEDITORDEPS+= $(SRC)/lunatic/mapster32.def
        EDITORLDFLAGS+= $(SRC)/lunatic/mapster32.def
    endif
    ifeq ($(SUBPLATFORM),LINUX)
        override STRIP=
        GAMELDFLAGS+= -Wl,--dynamic-list=$(SRC)/lunatic/dynsymlist
        EDITORLDFLAGS+= -Wl,--dynamic-list=$(SRC)/lunatic/dynsymlist_m32
    endif
endif


# PLATFORM SPECIFIC SETTINGS

ifeq ($(SUBPLATFORM),LINUX)
    ifeq (0,$(CLANG))
        OURCOMMONFLAGS	+= -fno-pic
    endif
    OURASFLAGS	+= -f elf
    LIBS		+= -lFLAC -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),DARWIN)
    OURCOMMONFLAGS += -fno-pic
    LIBDIRS += -L$(abspath $(JAUDIOLIBDIR)/third-party/Apple/lib)

    ifneq ($(findstring x86_64,$(ARCH)),x86_64)
        ifeq (,$(ARCH))
            ifneq ($(findstring x86_64,$(SYSARCH)),x86_64)
                LIBS += -read_only_relocs suppress
            endif
        else
            LIBS += -read_only_relocs suppress
        endif
    endif

    ifeq (1,$(SDL_FRAMEWORK))
        OURCOMMONFLAGS += -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers

        LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm \
                -Wl,-framework,SDL -Wl,-framework,SDL_mixer platform/Apple/lib/libSDLmain.a \
                -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
                -Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
                -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
                -Wl,-framework,QuickTime -lm \
                -Wl,-rpath -Wl,"@loader_path/../Frameworks"
        # We have SDLMain.m from the OSX SDL package in the Apple/ subdir:
        EDITOROBJS+=$(OBJ)/SDLMain.$o
        GAMEOBJS+=$(OBJ)/SDLMain.$o
    else
        OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
        LIBS += -lFLAC -lvorbisfile -lvorbis -logg -lm -lSDL_mixer \
                -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
                -Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
                -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
                -Wl,-framework,QuickTime -lm
    endif

    ifneq (0,$(OSX_STARTUPWINDOW))
        GAMEOBJS+=$(OBJ)/GrpFile.game.$o $(OBJ)/GameListSource.game.$o $(OBJ)/startosx.game.$o
    endif

    OURASFLAGS += -f macho
endif

ifeq ($(PLATFORM),WINDOWS)
    OURCOMMONFLAGS	+= -fno-pic -DUNDERSCORES
    OURASFLAGS+= -DUNDERSCORES -f win32
    LIBS += -lFLAC -lvorbisfile -lvorbis -logg
    LIBDIRS += -L$(abspath $(JAUDIOLIBDIR)/third-party/Windows/lib$(WINLIB))
    GAMEOBJS+= $(OBJ)/gameres.$o $(OBJ)/winbits.$o $(OBJ)/startwin.game.$o
    EDITOROBJS+= $(OBJ)/buildres.$o
    JAUDIOLIB=libjfaudiolib_win32.a
    ENETLIB=libenet_win32.a
    ifeq ($(RENDERTYPE),WIN)
        OURCOMMONFLAGS += -I$(DXROOT) -I$(DXROOT)/include
        LIBS+= -ldsound
        GAMEOBJS+= $(OBJ)/music.$o $(OBJ)/midi.$o $(OBJ)/mpu401.$o
    endif
endif
# -lGLU to build with gluBuild2DMipmaps
ifeq ($(RENDERTYPE),SDL)
    ifeq ($(PLATFORM),WINDOWS)
        OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
        LIBS+= platform/Windows/lib$(WINLIB)/SDL_mixer.lib
        LIBDIRS+= -L$(SDLROOT)/lib
    else
        ifneq ($(PLATFORM),DARWIN)
            LIBS+= -l$(SDLNAME)_mixer
        endif
    endif

    ifeq (1,$(HAVE_GTK2))
        OURCOMMONFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
        GAMEOBJS+= $(OBJ)/game_banner.$o $(OBJ)/startgtk.game.$o
        EDITOROBJS+= $(OBJ)/editor_banner.$o
    endif

    GAMEOBJS+= $(OBJ)/game_icon.$o $(OBJ)/sdlmusic.$o
    EDITOROBJS+= $(OBJ)/build_icon.$o
endif


OURCOMMONFLAGS+= $(BUILDCOMMONFLAGS)

ifeq ($(PLATFORM),WINDOWS)
    ifneq ($(findstring x86_64,$(COMPILERTARGET)),x86_64)
        PRINTLDFLAGS+= -Wl,--large-address-aware
    endif
endif
#ifneq (0,$(KRANDDEBUG))
ifeq ($(PLATFORM),DARWIN)
    PRINTLDFLAGS+=-Wl,-map -Wl,$@.memmap
else
    PRINTLDFLAGS+=-Wl,-Map=$@.memmap
endif
#endif
ifneq (0,$(PROFILER))
    PRINTLDFLAGS+=-pg
endif
ifeq ($(PLATFORM),WII)
    PRINTLDFLAGS+= -mrvl -meabi -mhard-float -Wl,--gc-sections -Wl,-Map,$(notdir $@).map
    # -msdata=eabi
endif

COMPILER=$(CC) $(OURCONLYFLAGS)
LINKER=$(L_CC)
ifneq ($(CPLUSPLUS),0)
    COMPILER=$(CXX) $(OURCXXFLAGS)
    LINKER=$(L_CXX)
endif

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean all engine $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB)

# TARGETS

UTILOBJS=$(OBJ)/ivfrate.$o
UTILS=ivfrate$(EXESUFFIX)

all: start $(EDUKE32_TARGET) $(MAPSTER32_TARGET) finish
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

utils: start $(UTILS) finish
	@ls -l $(UTILS)

start:
	$(BUILD_STARTED)

finish:
	$(BUILD_FINISHED)


$(EDUKE32): $(GAMEOBJS) $(EOBJ)/$(ENGINELIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENET_TARGET) $(MISCGAMEDEPS)
	$(LINK_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(GAMELDFLAGS) $(OURLIBS) $(STATICSTDCPP); then $(LINK_OK); else $(LINK_FAILED); fi
ifneq ($(STRIP),)
	$(STRIP) $(EDUKE32)
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/EDuke32.app" "./"
	mkdir -p "EDuke32.app/Contents/MacOS"
	cp -f "$(EDUKE32)" "EDuke32.app/Contents/MacOS/"
endif

$(MAPSTER32): $(EDITOROBJS) $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(MISCEDITORDEPS)
	$(LINK_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(EDITORLDFLAGS) $(OURLIBS) $(STATICSTDCPP); then $(LINK_OK); else $(LINK_FAILED); fi
ifneq ($(STRIP),)
	$(STRIP) $(MAPSTER32)
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "platform/Apple/bundles/Mapster32.app" "./"
	mkdir -p "Mapster32.app/Contents/MacOS"
	cp -f "$(MAPSTER32)" "Mapster32.app/Contents/MacOS/"
endif

include Makefile.deps

.PHONY: enginelib editorlib
enginelib editorlib:
	-mkdir -p $(EOBJ)
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(EROOT)\033[0;35m \033[0m\n"
endif
	$(MAKE) -C $(EROOT)/ "OBJ=../$(EOBJ)" $@ LUNATIC=$(LUNATIC)
ifeq ($(PRETTY_OUTPUT),1)
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	

$(EOBJ)/$(ENGINELIB): enginelib
$(EOBJ)/$(EDITORLIB): editorlib
$(JAUDIOLIBDIR)/$(JAUDIOLIB):
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(JAUDIOLIBDIR)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(JAUDIOLIBDIR)
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	

$(ENETDIR)/$(ENETLIB):
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(ENETDIR)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(ENETDIR)
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	


# RULES

$(EBACKTRACEDLL): platform/Windows/src/backtrace.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) -O2 -shared -Wall -Wextra -I$(EINC) -o $@ $^ -lbfd -liberty -limagehlp; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	$(AS) $(OURASFLAGS) $< -o $@

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

#### Utilities

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

ivfrate$(EXESUFFIX): $(OBJ)/ivfrate.$o
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(OURLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi

#### Lunatic

# Create object files directly with luajit
$(OBJ)/luaJIT_BC_%.$o: $(SRC)/lunatic/%.lua
	$(COMPILE_STATUS)
	if $(LUAJIT) -bg $< $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

# Same thing for defs.ilua which I'm too reluctant to rename now:
$(OBJ)/luaJIT_BC_%.$o: $(SRC)/lunatic/%.ilua
	if $(LUAJIT) -bg $< $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/lunatic/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

# TODO: _m32
# List of exported symbols, OS X
$(SRC)/lunatic/dynsymlist_osx: $(SRC)/lunatic/dynsymlist
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

#$(SRC)/lunatic/aliases_list: $(SRC)/lunatic/dynsymlist_osx
#	sed 's/_\([A-Za-z_][A-Za-z_0-9]*\)/_\1 \1/g' $< > $@

# List of exported symbols, Windows
$(SRC)/lunatic/eduke32.def: $(SRC)/lunatic/dynsymlist
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@

$(SRC)/lunatic/mapster32.def: $(SRC)/lunatic/dynsymlist_m32
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@

####

$(OBJ)/%.$o: platform/Apple/%.m
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.m
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cc
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cxx
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/jmact/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC) -DPOLYMER=$(POLYMER); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/game_banner.$o: $(RSRC)/game_banner.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -Wno-pointer-sign -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -Wno-pointer-sign -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(RSRC)/game_banner.c: $(RSRC)/game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES

clean:
	-rm -f $(OBJ)/* $(EDUKE32) $(EDUKE32).memmap $(MAPSTER32) $(MAPSTER32).memmap core* && $(MAKE) -C $(JAUDIOLIBDIR) clean && $(MAKE) -C $(ENETDIR) clean
ifeq ($(PLATFORM),DARWIN)
	-rm -rf EDuke32.app Mapster32.app
endif
	echo -n "" > $(OBJ)/keep.me

cleanutils:
	-rm -f $(UTILS) $(UTILOBJS) $(addsuffix .memmap, $(UTILS))

veryclean: clean cleanutils
	-rm -f $(EOBJ)/* $(RSRC)/*banner* $(EBACKTRACEDLL)
	echo -n "" > $(EOBJ)/keep.me

printutils:
	echo "$(UTILS)"
