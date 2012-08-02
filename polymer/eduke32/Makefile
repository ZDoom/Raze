#
# EDuke32 Makefile for GNU Make
#

include Makefile.common


# SDK locations - adjust to match your setup
ifneq ($(DXROOT_OVERRIDE),)
    DXROOT=$(DXROOT_OVERRIDE)
else
    DXROOT=../sdk/dx
    #DXROOT=c:/sdks/directx/dx8
endif

# Lunatic development, do not touch!
LUNATIC=0

# Build locations
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

OURCFLAGS=$(BASECFLAGS) \
    -I$(INC) -I$(EINC) -I$(SRC)/jmact -I$(JAUDIOLIBDIR)/include -I$(ENETDIR)/include -I$(SRC)/xdelta3

OURCXXFLAGS=$(BASECXXFLAGS)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURASFLAGS=$(BASEASFLAGS)
OURLDFLAGS=$(BASELDFLAGS) $(BUILDLDFLAGS)


JAUDIOLIBDIR=$(SRC)/jaudiolib
JAUDIOLIB=libjfaudiolib.a

ENETDIR=$(SRC)/enet
ENETLIB=libenet.a

EBACKTRACEDLL=ebacktrace1.dll
EBACKTRACEDLL_TARGET=


include $(EROOT)/Makefile.shared


ifneq (0,$(USE_LIBVPX))
    ifeq ($(PLATFORM),WINDOWS)
        LIBS+= -LWindows/lib
        OURCFLAGS+= -IWindows/include
    endif
    ifeq ($(PLATFORM),DARWIN)
        # vpx headers and lib from 'port install libvpx' or 'brew install libvpx'
        LIBS+= -L/opt/local/lib -L/usr/local/lib
        OURCFLAGS+= -I/opt/local/include -I/usr/local/include
    endif
endif

ifeq ($(PLATFORM),WINDOWS)
    OBJ=obj_win
    EOBJ=eobj_win
    LIBS+= $(L_SSP) -Wl,--enable-auto-import
    ifeq (0,$(RELEASE))
        EBACKTRACEDLL_TARGET=$(EBACKTRACEDLL)
    endif
else
    ifeq ($(PLATFORM),LINUX)
        LIBS+= -ldl -lrt
    endif
    LIBS+= -pthread
    ifneq (0,$(PROFILER))
        ifneq ($(PLATFORM),DARWIN)
            LIBS+= -lprofiler
        endif
        debug+= -pg
    endif
    OBJ=obj
    EOBJ=eobj
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
        $(OBJ)/xdelta3.$o \
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

MISCGAMEDEPS=


## Lunatic devel

LUAJIT=luajit

# for LJ headers:
LUAJIT_WIN_SRC:= g:/mod/LuaJIT-2.0.0-beta8/src

ifneq (0,$(LUNATIC))
    # TODO: pass these flags only when building EDuke32, not Mapster32

    ifeq ($(PLATFORM),WINDOWS)
        OURCFLAGS+= -I$(LUAJIT_WIN_SRC)
    else
        OURCFLAGS+= -I/usr/local/include/luajit-2.0
    endif
    OURCFLAGS+= -I$(SRC)/lunatic -DLUNATIC

    ifeq ($(PLATFORM),WINDOWS)
        LIBS+= -LWindows/lib -lluajit
    else
        LIBS+= -L/usr/local/lib -lluajit-5.1
    endif
    GAMEOBJS+= $(OBJ)/lunatic.$o

    GAMEOBJS+= $(OBJ)/../lpeg.$o  # TEMP
    GAMEOBJS+= $(OBJ)/luaJIT_BC_con_lang.$o \
               $(OBJ)/luaJIT_BC_lunacon.$o \
               $(OBJ)/luaJIT_BC_geom.$o \
               $(OBJ)/luaJIT_BC_randgen.$o \
               $(OBJ)/luaJIT_BC_stat.$o

    # now, take care of having the necessary symbols (sector, wall, etc.) in the
    # executable no matter what the debugging level

    ifeq ($(PLATFORM),DARWIN)
        # strip on OSX says: removing global symbols from a final linked no longer supported.
        #                    Use -exported_symbols_list at link time when building
        # But, following _their_ directions does not give us the symbols! wtf?
        STRIP+= -s $(SRC)/lunatic/dynsymlist_osx

        MISCGAMEDEPS+= $(SRC)/lunatic/dynsymlist_osx
        OURLDFLAGS+= -pagezero_size 10000 -image_base 100000000 #-Wl,-alias_list -Wl,$(SRC)/lunatic/aliases_list #-exported_symbols_list $(SRC)/lunatic/dynsymlist_osx
    endif
    ifeq ($(PLATFORM),WINDOWS)
        MISCGAMEDEPS+= $(SRC)/lunatic/eduke32.def
        OURLDFLAGS+= $(SRC)/lunatic/eduke32.def
    endif
    ifeq ($(PLATFORM),LINUX)
        OURLDFLAGS+= -Wl,--dynamic-list=$(SRC)/lunatic/dynsymlist
    endif
endif


# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
    ifeq (0,$(CLANG))
        OURCFLAGS	+= -fno-pic
    endif
    OURASFLAGS	+= -f elf
    LIBS		+= -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),WII)
    LIBS += -L$(LIBOGC_LIB) -L$(PORTLIBS)/lib -lvorbisidec -lfreetype -lfat -lwiiuse -lbte -logc -lm -lwiikeyboard
endif

ifeq ($(PLATFORM),DARWIN)
    OURCFLAGS += -fno-pic -I$(abspath Apple) -I$(abspath Apple/include)

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
        OURCFLAGS += -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers

        LIBS += -LApple/lib -lvorbisfile -lvorbis -logg -lm \
                -Wl,-framework,SDL -Wl,-framework,SDL_mixer Apple/lib/libSDLmain.a \
                -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
                -Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
                -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
                -Wl,-framework,QuickTime -lm \
                -Wl,-rpath -Wl,"@loader_path/../Frameworks"
        # We have SDLMain.m from the OSX SDL package in the Apple/ subdir:
        EDITOROBJS+=$(OBJ)/SDLMain.$o
        GAMEOBJS+=$(OBJ)/SDLMain.$o
    else
        OURCFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
        LIBS += -lvorbisfile -lvorbis -logg -lm -lSDL_mixer \
                -Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
                -Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
                -Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
                -Wl,-framework,QuickTime -lm
    endif

    ifneq (0,$(OSX_STARTUPWINDOW))
#        EDITOROBJS+=$(OBJ)/GrpFile.game.$o $(OBJ)/GameListSource.game.$o $(OBJ)/StartupWinController.game.$o
        GAMEOBJS+=$(OBJ)/GrpFile.game.$o $(OBJ)/GameListSource.game.$o $(OBJ)/StartupWinController.game.$o
    endif

    OURASFLAGS += -f macho
endif

ifeq ($(PLATFORM),WINDOWS)
    OURCFLAGS	+= -fno-pic -DUNDERSCORES -I$(DXROOT)/include
    OURASFLAGS+= -DUNDERSCORES -f win32
    LIBS		+= -L$(JAUDIOLIBDIR)/third-party/mingw32/lib -lvorbisfile -lvorbis -logg -lwsock32 -lws2_32 -lwinmm -ldsound
    GAMEOBJS+= $(OBJ)/gameres.$o $(OBJ)/winbits.$o $(OBJ)/startwin.game.$o $(OBJ)/music.$o $(OBJ)/midi.$o $(OBJ)/mpu401.$o
    EDITOROBJS+= $(OBJ)/buildres.$o
    JAUDIOLIB=libjfaudiolib_win32.a
    ENETLIB=libenet_win32.a
else
# -lGLU to build with gluBuild2DMipmaps
    ifeq ($(RENDERTYPE),SDL)
        ifeq (0,$(SDL_FRAMEWORK))
            ifneq ($(PLATFORM),WII)
                OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell $(SDLCONFIG) --cflags))
            endif
            LIBS+= -l$(SDLNAME)_mixer
        else
            OURCFLAGS   += -DSDL_FRAMEWORK
        endif

        ifeq (1,$(HAVE_GTK2))
            OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
            GAMEOBJS+= $(OBJ)/game_banner.$o $(OBJ)/startgtk.game.$o
            EDITOROBJS+= $(OBJ)/editor_banner.$o
        endif

        GAMEOBJS+= $(OBJ)/game_icon.$o $(OBJ)/sdlmusic.$o
        EDITOROBJS+= $(OBJ)/build_icon.$o
    endif
endif


OURCFLAGS+= $(BUILDCFLAGS)

ifeq ($(PLATFORM),WINDOWS)
    OURLDFLAGS+= -Wl,--large-address-aware
endif
#ifneq (0,$(KRANDDEBUG))
ifeq ($(PLATFORM),DARWIN)
    OURLDFLAGS+=-Wl,-map -Wl,$@.memmap
else
    OURLDFLAGS+=-Wl,-Map=$@.memmap
endif
#endif
ifneq (0,$(PROFILER))
    OURLDFLAGS+=-pg
endif
ifeq ($(PLATFORM),WII)
    OURLDFLAGS+= -mrvl -meabi -mhard-float -Wl,--gc-sections -Wl,-Map,$(notdir $@).map
    # -msdata=eabi
endif

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean all engine $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB)

EDUKE32_TARGET:=eduke32$(EXESUFFIX)
ifneq ($(PLATFORM),WII)
    MAPSTER32_TARGET:=mapster32$(EXESUFFIX)
endif

# TARGETS

all: notice $(EDUKE32_TARGET) $(MAPSTER32_TARGET) $(EBACKTRACEDLL_TARGET)

all:
	$(BUILD_FINISHED)
ifneq (,$(EDUKE32_TARGET))
	@ls -l $(EDUKE32_TARGET)
endif
ifneq (,$(MAPSTER32_TARGET))
	@ls -l $(MAPSTER32_TARGET)
endif

notice:
	$(BUILD_STARTED)

eduke32$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)/$(ENGINELIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB) $(MISCGAMEDEPS)
	$(LINK_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -o $@ $^ $(OURLDFLAGS) $(LIBS) $(STATICSTDCPP) $(STDCPPLIB); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))
  ifeq (0,$(DEBUGANYWAY))
    ifneq ($(PLATFORM),WII)
	  $(STRIP) $(EDUKE32_TARGET)
    endif
  endif
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "Apple/bundles/EDuke32.app" "./"
	mkdir -p "EDuke32.app/Contents/MacOS"
	cp -f "$(EDUKE32_TARGET)" "EDuke32.app/Contents/MacOS/"
endif

mapster32$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)/$(EDITORLIB) $(EOBJ)/$(ENGINELIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB)
	$(LINK_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -o $@ $^ $(OURLDFLAGS) $(LIBS) $(STATICSTDCPP) $(STDCPPLIB); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))	
  ifeq (0,$(DEBUGANYWAY))
    ifneq ($(PLATFORM),WII)
	  $(STRIP) $(MAPSTER32_TARGET)
    endif
  endif
endif
ifeq ($(PLATFORM),DARWIN)
	cp -RPf "Apple/bundles/Mapster32.app" "./"
	mkdir -p "Mapster32.app/Contents/MacOS"
	cp -f "$(MAPSTER32_TARGET)" "Mapster32.app/Contents/MacOS/"
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
$(EBACKTRACEDLL): Windows/src/backtrace.c
	if $(CC) -O2 -shared -Wall -Wextra -I$(EINC) -o $@ $^ -lbfd -liberty -limagehlp; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	$(AS) $(OURASFLAGS) $< -o $@

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

# Create object files directly with luajit
$(OBJ)/luaJIT_BC_%.$o: $(SRC)/lunatic/%.lua
	$(COMPILE_STATUS)
	if $(LUAJIT) -bg $< $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/lunatic/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/xdelta3/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(SRC)/lunatic/dynsymlist_osx: $(SRC)/lunatic/dynsymlist
	sed 's/[{};]//g;s/[A-Za-z_][A-Za-z_0-9]*/_&/g' $< > $@

$(SRC)/lunatic/aliases_list: $(SRC)/lunatic/dynsymlist_osx
	sed 's/_\([A-Za-z_][A-Za-z_0-9]*\)/_\1 \1/g' $< > $@

$(SRC)/lunatic/eduke32.def: $(SRC)/lunatic/dynsymlist
	echo EXPORTS > $@
	sed 's/[{};]//g' $< >> $@

$(OBJ)/%.$o: Apple/%.m
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cc
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cxx
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/jmact/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC) -DPOLYMER=$(POLYMER); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/game_banner.$o: $(RSRC)/game_banner.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -Wno-pointer-sign -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -Wno-pointer-sign -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(RSRC)/game_banner.c: $(RSRC)/game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/* eduke32$(EXESUFFIX) eduke32$(EXESUFFIX).memmap mapster32$(EXESUFFIX) mapster32$(EXESUFFIX).memmap core* duke3d_w32$(EXESUFFIX) && $(MAKE) -C $(JAUDIOLIBDIR) clean && $(MAKE) -C $(ENETDIR) clean
ifeq ($(PLATFORM),DARWIN)
	-rm -rf EDuke32.app Mapster32.app
endif
	echo -n "" > $(OBJ)/keep.me

veryclean: clean
	-rm -f $(EOBJ)/* $(RSRC)/*banner* $(EBACKTRACEDLL)
	echo -n "" > $(EOBJ)/keep.me
