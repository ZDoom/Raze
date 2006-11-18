# EDuke32 Makefile for GNU Make

# SDK locations - adjust to match your setup
# DXROOT=c:/sdks/msc/dx61

# Engine options
SUPERBUILD = 1
POLYMOST = 1
USE_OPENGL = 1
NOASM = 0

# Debugging options
RELEASE?=1
JFAUD?=0

# build locations

SRC=source
RSRC=rsrc
EROOT=../build
ESRC=$(EROOT)/src
EINC=$(EROOT)/include
INC=$(SRC)
o=o

ifneq (0,$(RELEASE))
  # debugging disabled
  debug=-fomit-frame-pointer -O2
else
  # debugging enabled
  debug=-ggdb -O0 -DDEBUGGINGAIDS
endif

CC=gcc
CXX=g++
# -Werror-implicit-function-declaration
OURCFLAGS=$(debug) -W -Wall -Wimplicit -Wno-char-subscripts -Wunused \
	-fno-pic -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS -DNOCOPYPROTECT \
	-I$(INC) -I$(EINC) -I$(SRC)/jmact -I$(SRC)/jaudiolib -I../jfaud/inc
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=-lm
ifneq (0,$(JFAUD))
	JFAUDLIBS=../jfaud/libjfaud.a ../jfaud/mpadec/libmpadec.a
	OURCFLAGS+=-DJFAUD
endif

NASMFLAGS=-s #-g
EXESUFFIX=

include $(EROOT)/Makefile.shared

ifeq ($(PLATFORM),LINUX)
	OBJ=obj.nix
	EOBJ=eobj.nix
	NASMFLAGS+= -f elf
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
	$(OBJ)/scriplib.$o

AUDIOLIB_FX_STUB=$(OBJ)/audiolib_fxstub.$o
AUDIOLIB_MUSIC_STUB=$(OBJ)/audiolib_musicstub.$o

AUDIOLIB_FX_SDL=$(OBJ)/mv_mix.$o \
	  $(OBJ)/mv_mix16.$o \
	  $(OBJ)/mvreverb.$o \
	  $(OBJ)/ll_man.$o \
	  $(OBJ)/fx_man.$o \
	  $(OBJ)/dsl.$o \
	  $(OBJ)/pitch.$o \
	  $(OBJ)/multivoc.$o
AUDIOLIB_MUSIC_SDL=$(OBJ)/sdlmusic.$o \
	  $(OBJ)/unixglob.$o

AUDIOLIB_JFAUD=$(OBJ)/jfaud_sounds.$o

AUDIOLIB_FX=$(OBJ)/mv_mix.$o \
	  $(OBJ)/mv_mix16.$o \
	  $(OBJ)/mvreverb.$o \
	  $(OBJ)/pitch.$o \
	  $(OBJ)/multivoc.$o \
	  $(OBJ)/ll_man.$o \
	  $(OBJ)/fx_man.$o \
	  $(OBJ)/dsoundout.$o
AUDIOLIB_MUSIC=$(OBJ)/midi.$o \
	  $(OBJ)/mpu401.$o \
	  $(OBJ)/music.$o

GAMEOBJS=$(OBJ)/game.$o \
	$(OBJ)/actors.$o \
	$(OBJ)/anim.$o \
	$(OBJ)/animlib.$o \
	$(OBJ)/config.$o \
	$(OBJ)/gamedef.$o \
	$(OBJ)/gameexec.$o \
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

EDITOROBJS=$(OBJ)/astub.$o

ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include
	NASMFLAGS+= -DUNDERSCORES -f win32
	GAMEOBJS+= $(OBJ)/gameres.$o $(OBJ)/winbits.$o $(OBJ)/startwin.game.$o
	EDITOROBJS+= $(OBJ)/buildres.$o
endif

ifeq ($(RENDERTYPE),SDL)
	 OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell sdl-config --cflags))

	ifneq (0,$(JFAUD))
		AUDIOLIBOBJ=$(AUDIOLIB_JFAUD)
	else
		ifneq ($(PLATFORM),WINDOWS)
			AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_SDL) $(AUDIOLIB_FX_SDL) $(OBJ)/sounds.$o
			LIBS+= -lSDL_mixer
		else
			AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB) $(OBJ)/sounds.$o
		endif
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
	ifneq (0,$(JFAUD))
		AUDIOLIBOBJ=$(AUDIOLIB_JFAUD)
	else
		AUDIOLIBOBJ=$(AUDIOLIB_MUSIC) $(AUDIOLIB_FX) $(OBJ)/sounds.$o
	endif
endif

GAMEOBJS+= $(AUDIOLIBOBJ)
OURCFLAGS+= $(BUILDCFLAGS)
OURCXXFLAGS+= $(BUILDCFLAGS)

.PHONY: clean all engine $(EOBJ)/$(ENGINELIB) $(EOBJ)/$(EDITORLIB)

# TARGETS

# Invoking Make from the terminal in OSX just chains the build on to xcode
ifeq ($(PLATFORM),DARWIN)
ifeq ($(RELEASE),0)
style=Development
else
style=Deployment
endif
.PHONY: alldarwin
alldarwin:
	cd osx && xcodebuild -target All -buildstyle $(style)
endif

ifeq ($(PLATFORM),WINDOWS) 
all: eduke32$(EXESUFFIX) mapster32$(EXESUFFIX) duke3d_w32$(EXESUFFIX) 
else
all: eduke32$(EXESUFFIX) mapster32$(EXESUFFIX)
endif

eduke32$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)/$(ENGINELIB)
	$(CC) -o $@ $^ $(JFAUDLIBS) $(LIBS) $(STDCPPLIB) -Wl,-Map=$@.map
	-rm eduke32.sym$(EXESUFFIX)
	cp eduke32$(EXESUFFIX) eduke32.sym$(EXESUFFIX)
	strip eduke32$(EXESUFFIX)
	
mapster32$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)/$(EDITORLIB) $(EOBJ)/$(ENGINELIB)
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) -Wl,-Map=$@.map
	-rm mapster32.sym$(EXESUFFIX)
	cp mapster32$(EXESUFFIX) mapster32.sym$(EXESUFFIX)
	strip mapster32$(EXESUFFIX)

duke3d_w32$(EXESUFFIX): $(OBJ)/wrapper.$o
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ -Wl
	strip duke3d_w32$(EXESUFFIX)

include Makefile.deps

.PHONY: enginelib editorlib
enginelib editorlib:
	-mkdir $(EOBJ)
	$(MAKE) -C $(EROOT)/ "OBJ=$(CURDIR)/$(EOBJ)" \
		SUPERBUILD=$(SUPERBUILD) POLYMOST=$(POLYMOST) \
		USE_OPENGL=$(USE_OPENGL) \
		NOASM=$(NOASM) RELEASE=$(RELEASE) $@
	
$(EOBJ)/$(ENGINELIB): enginelib
$(EOBJ)/$(EDITORLIB): editorlib

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	nasm $(NASMFLAGS) $< -o $@
$(OBJ)/%.$o: $(SRC)/jaudiolib/%.nasm
	nasm $(NASMFLAGS) $< -o $@
 
$(OBJ)/%.$o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)/%.$o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)/%.$o: $(SRC)/jmact/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)/%.$o: $(SRC)/jaudiolib/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1

ifeq (1,$(JFAUD))
$(OBJ)/%.$o: $(SRC)/misc/%.rc
	windres -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC) -DJFAUD
else
$(OBJ)/%.$o: $(SRC)/misc/%.rc
	windres -i $< -o $@ --include-dir=$(EINC) --include-dir=$(SRC)
endif

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
 
$(OBJ)/%.$o: $(RSRC)/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
 
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
ifeq ($(PLATFORM),DARWIN)
	cd osx && xcodebuild -target All clean
else
	-rm -f $(OBJ)/* eduke32$(EXESUFFIX) eduke32.sym$(EXESUFFIX) mapster32$(EXESUFFIX) mapster32.sym$(EXESUFFIX) core* duke3d_w32$(EXESUFFIX)
endif

veryclean: clean
ifeq ($(PLATFORM),DARWIN)
else
	-rm -f $(EOBJ)/*
endif
