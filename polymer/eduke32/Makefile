#
# EDuke32 Makefile for GNU Make
#

# DEFINES

# Use colored output
PRETTY_OUTPUT = 1

# SDK locations - adjust to match your setup
DXROOT=../sdk/dx

ALROOT		= AL

# Engine options
SUPERBUILD = 1
POLYMOST = 1
POLYMER = 0
USE_OPENGL = 1
NOASM = 0
LINKED_GTK = 0
BUILD32_ON_64 = 0
RANCID_NETWORKING = 1

# Debugging/Build options
RELEASE?=0
NOSOUND?=0
USE_OPENAL	?= 1
OPTLEVEL?=2

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
  debug=-fomit-frame-pointer -funswitch-loops -O$(OPTLEVEL)
else
	# Debugging enabled
  debug=-ggdb -O0 -DDEBUGGINGAIDS
endif

CC=gcc
CXX=g++
AS=nasm

GCC_MAJOR    := $(shell $(CC) -dumpversion 2>&1 | cut -d'.' -f1)
GCC_MINOR    := $(shell $(CC) -dumpversion 2>&1 | cut -d'.' -f2)

ifeq (4,$(GCC_MAJOR))
  ifneq (0,$(GCC_MINOR))
#    debug += -ftree-vectorize
    ifneq (1,$(GCC_MINOR))
      ifneq (2,$(GCC_MINOR))
#        debug += -finline-small-functions -fpredictive-commoning
      endif
    endif
  endif
endif

OURCFLAGS=$(debug) -W -Wall -Wimplicit -Werror-implicit-function-declaration \
		-funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS -DNOCOPYPROTECT \
	    -I$(INC) -I$(EINC) -I$(SRC)/jmact -I$(SRC)/jaudiolib -D_FORTIFY_SOURCE=2 \
#        -march=pentium3 -mtune=generic -mmmx -m3dnow -msse -mfpmath=sse
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=-lm

NASMFLAGS	= -s
EXESUFFIX=

include $(EROOT)/Makefile.shared

    ifeq ($(PLATFORM),WINDOWS)
	    OBJ=obj_win
	    EOBJ=eobj_win
    else
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

AUDIOLIB_FX_STUB=$(OBJ)/audiolib_fxstub.$o
AUDIOLIB_MUSIC_STUB=$(OBJ)/audiolib_musicstub.$o

AUDIOLIB_FX_SDL=$(OBJ)/ll_man.$o \
	  $(OBJ)/fx_man.$o \
	  $(OBJ)/dsl.$o \
	  $(OBJ)/pitch.$o \
      $(OBJ)/multivoc.$o

AUDIOLIB_MUSIC_SDL=$(OBJ)/sdlmusic.$o

AUDIOLIB_FX=$(OBJ)/pitch.$o \
	  $(OBJ)/multivoc.$o \
	  $(OBJ)/ll_man.$o \
	  $(OBJ)/fx_man.$o \
      $(OBJ)/dsoundout.$o
      
ifeq ($(USE_OPENAL),1)
	AUDIOLIB_FX += $(OBJ)/openal.$o
endif
      
AUDIOLIB_MUSIC=$(OBJ)/midi.$o \
	  $(OBJ)/mpu401.$o \
	  $(OBJ)/music.$o

ifeq (0,$(NOASM))
  # Assembly sound mixing code
  AUDIOLIB_FX_SDL += $(OBJ)/mv_mix.$o $(OBJ)/mv_mix16.$o $(OBJ)/mvreverb.$o
  AUDIOLIB_FX     += $(OBJ)/mv_mix.$o $(OBJ)/mv_mix16.$o $(OBJ)/mvreverb.$o
else
  # C fallbacks for sound mixing code
  AUDIOLIB_FX_SDL += $(OBJ)/mv_mix-c.$o $(OBJ)/mvreverb-c.$o
  AUDIOLIB_FX     += $(OBJ)/mv_mix-c.$o $(OBJ)/mvreverb-c.$o
endif


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
	$(OBJ)/testcd.$o \
	$(OBJ)/osdfuncs.$o \
	$(OBJ)/osdcmds.$o \
	$(OBJ)/grpscan.$o \
	$(JMACTOBJ)

EDITOROBJS=$(OBJ)/astub.$o \
    $(OBJ)/mathutil.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
	OURCFLAGS	+= -fno-pic
	NASMFLAGS	+= -f elf
	LIBS		+= -lvorbisfile -lvorbis -logg 
	USE_OPENAL	= 0
endif

ifeq ($(PLATFORM),DARWIN)
	ALROOT = Apple
	ifeq (1,$(SDL_FRAMEWORK))
		OURCFLAGS	+= -fno-pic -I$(ALROOT)/include -I/Library/Frameworks/SDL.framework/Headers \
				-I-I/Library/Frameworks/SDL_mixer.framework/Headers
		LIBS		+= -read_only_relocs suppress -L$(ALROOT)/lib -lvorbisfile -lvorbis -logg -lm \
				-Wl,-framework,SDL -Wl,-framework,SDL_mixer $(ALROOT)/lib/libSDLmain.a \
				-Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
				-Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
				-Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
				-Wl,-framework,QuickTime -lm

	else
		OURCFLAGS	+= -fno-pic -I$(ALROOT)/include -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
		LIBS		+= -read_only_relocs suppress -L$(ALROOT)/lib -lvorbisfile -lvorbis -logg -lm -lSDL_mixer \
				-Wl,-framework,Cocoa -Wl,-framework,Carbon -Wl,-framework,OpenGL \
				-Wl,-framework,CoreMidi -Wl,-framework,AudioUnit \
				-Wl,-framework,AudioToolbox -Wl,-framework,IOKit -Wl,-framework,AGL \
				-Wl,-framework,QuickTime -lm
	endif
	NASMFLAGS	+= -f Mach-O
endif

ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS	+= -fno-pic -DUNDERSCORES -I$(DXROOT)/include -I$(ALROOT)/include # -I$(ENETROOT)/include
	NASMFLAGS+= -DUNDERSCORES -f win32
	LIBS		+= -L$(ALROOT)/lib -lvorbisfile -lvorbis -logg -lwsock32 -lws2_32 -lwinmm #-L$(ENETROOT)
	GAMEOBJS+= $(OBJ)/gameres.$o $(OBJ)/winbits.$o $(OBJ)/startwin.game.$o
	EDITOROBJS+= $(OBJ)/buildres.$o
endif

ifeq ($(RENDERTYPE),SDL)
	ifeq (0,$(SDL_FRAMEWORK))
		OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell $(SDLCONFIG) --cflags))
	else
		OURCFLAGS	+= -DSDL_FRAMEWORK
	endif

		ifneq ($(PLATFORM),WINDOWS)
			AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_SDL) $(AUDIOLIB_FX_SDL) $(OBJ)/sounds.$o
			AUDIOLIBOBJ_M32=$(AUDIOLIB_FX_SDL) $(OBJ)/sounds_mapster32.$o
		ifeq (0,$(SDL_FRAMEWORK))
			LIBS+= -lSDL_mixer
		endif
		else
			AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB) $(OBJ)/sounds.$o
			AUDIOLIBOBJ_M32=$(AUDIOLIB_FX_STUB) $(OBJ)/sounds_mapster32.$o
		endif

	ifeq (1,$(HAVE_GTK2))
		OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		GAMEOBJS+= $(OBJ)/game_banner.$o $(OBJ)/startgtk.game.$o
		EDITOROBJS+= $(OBJ)/editor_banner.$o
	endif

	GAMEOBJS+= $(OBJ)/game_icon.$o
	EDITOROBJS+= $(OBJ)/build_icon.$o
endif

ifeq ($(RENDERTYPE),WIN)
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC) $(AUDIOLIB_FX) $(OBJ)/sounds.$o
    AUDIOLIBOBJ_M32=$(AUDIOLIB_FX) $(OBJ)/sounds_mapster32.$o
endif

ifeq ($(NOSOUND),1)
    AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB) $(OBJ)/sounds.$o
    AUDIOLIBOBJ_M32=$(AUDIOLIB_FX_STUB) $(OBJ)/sounds_mapster32.$o
endif

GAMEOBJS+= $(AUDIOLIBOBJ)
EDITOROBJS+= $(AUDIOLIBOBJ_M32)
OURCFLAGS+= $(BUILDCFLAGS)
OURCXXFLAGS+= $(BUILDCFLAGS)

ifeq ($(USE_OPENAL),1)
    OURCFLAGS+= -DUSE_OPENAL
endif

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean all engine $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB)

# TARGETS

all: notice eduke32$(EXESUFFIX) mapster32$(EXESUFFIX)

all:
	$(BUILD_FINISHED)
	@ls -l eduke32$(EXESUFFIX)
	@ls -l mapster32$(EXESUFFIX)

notice:
	$(BUILD_STARTED)
	
eduke32$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)/$(ENGINELIB)
	$(LINK_STATUS)
	if $(CC) -o $@ $^ $(LIBS) $(STDCPPLIB); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))
	strip eduke32$(EXESUFFIX)
endif
	
mapster32$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)/$(EDITORLIB) $(EOBJ)/$(ENGINELIB)
	$(LINK_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) $(STDCPPLIB); then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))	
	strip mapster32$(EXESUFFIX)
endif	

duke3d_w32$(EXESUFFIX): $(OBJ)/wrapper.$o
	$(LINK_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ -Wl; then $(LINK_OK); else $(LINK_FAILED); fi
ifeq (1,$(RELEASE))	
	strip duke3d_w32$(EXESUFFIX)
endif	

include Makefile.deps

ifneq (0,$(RANCID_NETWORKING))
	duke3d_h+=$(EINC)/mmulti_unstable.h
else
	duke3d_h+=$(EINC)/mmulti.h
endif

.PHONY: enginelib editorlib
enginelib editorlib:
	-mkdir -p $(EOBJ)
ifeq ($(PRETTY_OUTPUT),1)	
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)/$(EROOT)\033[0;35m \033[0m\n"
endif	
	$(MAKE) -C $(EROOT)/ "OBJ=../$(EOBJ)" \
		SUPERBUILD=$(SUPERBUILD) POLYMOST=$(POLYMOST) \
		USE_OPENGL=$(USE_OPENGL) BUILD32_ON_64=$(BUILD32_ON_64) \
		NOASM=$(NOASM) RELEASE=$(RELEASE) OPTLEVEL=$(OPTLEVEL) RANCID_NETWORKING=$(RANCID_NETWORKING) $@
ifeq ($(PRETTY_OUTPUT),1)		
	printf "\033[K\033[0;35mChanging dir to \033[1;35m$(CURDIR)\033[0;35m \033[0m\n"
endif	
	
$(EOBJ)/$(ENGINELIB): enginelib
$(EOBJ)/$(EDITORLIB): editorlib

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	$(AS) $(NASMFLAGS) $< -o $@
$(OBJ)/%.$o: $(SRC)/jaudiolib/%.nasm
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
$(OBJ)/%.$o: $(SRC)/jaudiolib/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if windres -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

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
	-rm -f $(OBJ)/* eduke32$(EXESUFFIX) mapster32$(EXESUFFIX) core* duke3d_w32$(EXESUFFIX)

veryclean: clean
	-rm -f $(EOBJ)/*
