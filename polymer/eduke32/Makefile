#
# EDuke32 Makefile for GNU Make
#

# DEFINES

# Use colored output
PRETTY_OUTPUT = 1

# SDK locations - adjust to match your setup
DXROOT=../sdk/dx

# Engine options
SUPERBUILD = 1
POLYMOST = 1
POLYMER = 1
USE_OPENGL = 1
NOASM = 0
LINKED_GTK = 0
BUILD32_ON_64 = 0
NEDMALLOC = 1

# Debugging/Build options
RELEASE?=1
DEBUGANYWAY?=0
KRANDDEBUG?=0
NOSOUND?=0
OPTLEVEL?=2
PROFILER?=0

ifneq (0,$(KRANDDEBUG))
    RELEASE=0
endif

# Build locations
SRC=source
RSRC=rsrc
EROOT=build
ESRC=$(EROOT)/src
EINC=$(EROOT)/include
INC=$(SRC)
o=o
# ENETROOT=$(ESRC)/enet

ifneq (0,$(RELEASE))
	# Debugging disabled
    debug=-fomit-frame-pointer -funswitch-loops -O$(OPTLEVEL) -fno-stack-protector
    LIBS=-lm
else
	# Debugging enabled
    debug=-ggdb -O0 -DDEBUGGINGAIDS -fstack-protector-all
    LIBS=-lm -lssp -Wl,--enable-auto-import
    ifneq (0,$(KRANDDEBUG))
      debug+=-fno-inline -fno-inline-functions -fno-inline-functions-called-once
      debug+=-DKRANDDEBUG=1
    endif
endif

ifneq (0,$(DEBUGANYWAY))
  debug+=-ggdb
endif

JAUDIOLIBDIR=$(SRC)/jaudiolib
JAUDIOLIB=libjfaudiolib.a

ENETDIR=$(SRC)/enet
ENETLIB=libenet.a

CC=gcc
CXX=g++
AS=nasm
RC=windres
STRIP=strip

OURCFLAGS=$(debug) -W -Wall -Wimplicit -Werror-implicit-function-declaration \
	-funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-I$(INC) -I$(EINC) -I$(SRC)/jmact -I$(JAUDIOLIBDIR)/include -I$(ENETDIR)/include -D_FORTIFY_SOURCE=2 \
	-fjump-tables $(ARCH)
	
OURCXXFLAGS=-fno-exceptions -fno-rtti

NASMFLAGS	= -s
EXESUFFIX=

include $(EROOT)/Makefile.shared

ifeq ($(PLATFORM),WINDOWS)
    OBJ=obj_win
    EOBJ=eobj_win
else
    LIBS+= -ldl -pthread
    ifeq (1,$(PROFILER))
        LIBS+= -lprofiler
    endif
  	OBJ=obj
   	EOBJ=eobj
endif

JMACTOBJ=$(OBJ)/util_lib.$o \
	$(OBJ)/file_lib.$o \
	$(OBJ)/control.$o \
	$(OBJ)/keyboard.$o \
	$(OBJ)/mouse.$o \
	$(OBJ)/mathutil.$o \
	$(OBJ)/scriplib.$o \
	$(OBJ)/animlib.$o

GAMEOBJS=$(OBJ)/game.$o \
	$(OBJ)/actors.$o \
	$(OBJ)/anim.$o \
	$(OBJ)/config.$o \
	$(OBJ)/gamedef.$o \
	$(OBJ)/gameexec.$o \
	$(OBJ)/gamevars.$o \
	$(OBJ)/global.$o \
	$(OBJ)/menus.$o \
	$(OBJ)/namesdyn.$o \
	$(OBJ)/player.$o \
	$(OBJ)/premap.$o \
	$(OBJ)/savegame.$o \
	$(OBJ)/sector.$o \
	$(OBJ)/rts.$o \
	$(OBJ)/osdfuncs.$o \
	$(OBJ)/osdcmds.$o \
	$(OBJ)/grpscan.$o \
	$(OBJ)/sounds.$o \
	$(JMACTOBJ)

EDITOROBJS=$(OBJ)/astub.$o \
	$(OBJ)/m32def.$o \
	$(OBJ)/m32exec.$o \
	$(OBJ)/m32vars.$o \
	$(OBJ)/mathutil.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
	OURCFLAGS	+= -fno-pic
	NASMFLAGS	+= -f elf
	LIBS		+= -lvorbisfile -lvorbis -logg
endif

ifeq ($(PLATFORM),DARWIN)
	ifeq (1,$(SDL_FRAMEWORK))
		OURCFLAGS	+= -fno-pic -IApple/include -I/Library/Frameworks/SDL.framework/Headers \
				-I-I/Library/Frameworks/SDL_mixer.framework/Headers
		LIBS		+= -read_only_relocs suppress -LApple/lib -lvorbisfile -lvorbis -logg -lm \
				-Wl,-framework,SDL -Wl,-framework,SDL_mixer Apple/lib/libSDLmain.a \
				-Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
				-Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
				-Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
				-Wl,-framework,QuickTime -lm

	else
		OURCFLAGS	+= -fno-pic -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
		LIBS		+= -read_only_relocs suppress -lvorbisfile -lvorbis -logg -lm -lSDL_mixer \
				-Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
				-Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
				-Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
				-Wl,-framework,QuickTime -lm
	endif
	NASMFLAGS	+= -f Mach-O
endif

ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS	+= -fno-pic -DUNDERSCORES -I$(DXROOT)/include
	NASMFLAGS+= -DUNDERSCORES -f win32
	LIBS		+= -L$(JAUDIOLIBDIR)/third-party/mingw32/lib -lvorbisfile -lvorbis -logg -lwsock32 -lws2_32 -lwinmm -ldsound
	GAMEOBJS+= $(OBJ)/gameres.$o $(OBJ)/winbits.$o $(OBJ)/startwin.game.$o $(OBJ)/music.$o $(OBJ)/midi.$o $(OBJ)/mpu401.$o
	EDITOROBJS+= $(OBJ)/buildres.$o
    JAUDIOLIB=libjfaudiolib_win32.a
    ENETLIB=libenet_win32.a
else
    ifeq ($(RENDERTYPE),SDL)
    	ifeq (0,$(SDL_FRAMEWORK))
    		OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell $(SDLCONFIG) --cflags))
            LIBS+= -lSDL_mixer
    	else
    		OURCFLAGS	+= -DSDL_FRAMEWORK
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


EDITOROBJS+= $(OBJ)/sounds_mapster32.$o
OURCFLAGS+= $(BUILDCFLAGS)
OURCXXFLAGS+= $(BUILDCFLAGS)

MISCLINKOPTS=
ifneq (0,$(KRANDDEBUG))
  MISCLINKOPTS=-Wl,-Map=$@.memmap
endif

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean all engine $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB)

# TARGETS

all: notice eduke32$(EXESUFFIX) mapster32$(EXESUFFIX)

all:
	$(BUILD_FINISHED)
	@ls -l eduke32$(EXESUFFIX)
	@ls -l mapster32$(EXESUFFIX)

notice:
	$(BUILD_STARTED)

eduke32$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)/$(ENGINELIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB)
	$(LINK_STATUS)
	if $(CC) -o $@ $^ $(LIBS) $(STDCPPLIB) $(MISCLINKOPTS); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))
  ifeq (0,$(DEBUGANYWAY))
	$(STRIP) eduke32$(EXESUFFIX)
  endif
endif

mapster32$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)/$(EDITORLIB) $(EOBJ)/$(ENGINELIB) $(JAUDIOLIBDIR)/$(JAUDIOLIB) $(ENETDIR)/$(ENETLIB)
	$(LINK_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) $(STDCPPLIB) $(MISCLINKOPTS); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))	
  ifeq (0,$(DEBUGANYWAY))
	$(STRIP) mapster32$(EXESUFFIX)
  endif
endif	

include Makefile.deps

.PHONY: enginelib editorlib
enginelib editorlib:
	-mkdir -p $(EOBJ)
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(EROOT)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(EROOT)/ "OBJ=../$(EOBJ)" \
		SUPERBUILD=$(SUPERBUILD) POLYMOST=$(POLYMOST) DEBUGANYWAY=$(DEBUGANYWAY) KRANDDEBUG=$(KRANDDEBUG)\
		USE_OPENGL=$(USE_OPENGL) BUILD32_ON_64=$(BUILD32_ON_64) \
		NOASM=$(NOASM) RELEASE=$(RELEASE) OPTLEVEL=$(OPTLEVEL) $@
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	

$(EOBJ)/$(ENGINELIB): enginelib
$(EOBJ)/$(EDITORLIB): editorlib
$(JAUDIOLIBDIR)/$(JAUDIOLIB):
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(JAUDIOLIBDIR)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(JAUDIOLIBDIR) PRETTY_OUTPUT=$(PRETTY_OUTPUT) EROOT=$(EROOT) RELEASE=$(RELEASE) OPTLEVEL=$(OPTLEVEL) DEBUGANYWAY=$(DEBUGANYWAY)
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	

$(ENETDIR)/$(ENETLIB):
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(ENETDIR)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(ENETDIR) PRETTY_OUTPUT=$(PRETTY_OUTPUT) EROOT=$(EROOT) RELEASE=$(RELEASE) OPTLEVEL=$(OPTLEVEL) 
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	


# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	$(AS) $(NASMFLAGS) $< -o $@
 
$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/jmact/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC) -DPOLYMER=$(POLYMER); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/game_banner.$o: $(RSRC)/game_banner.c
$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
$(RSRC)/game_banner.c: $(RSRC)/game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/* eduke32$(EXESUFFIX) mapster32$(EXESUFFIX) core* duke3d_w32$(EXESUFFIX) && $(MAKE) -C $(JAUDIOLIBDIR) clean && $(MAKE) -C $(ENETDIR) clean

veryclean: clean
	-rm -f $(EOBJ)/* $(RSRC)/*banner*
