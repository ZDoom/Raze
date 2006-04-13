# EDuke32 Makefile for GNU Make

# SDK locations - adjust to match your setup
# DXROOT=c:/sdks/msc/dx61

# Engine options
SUPERBUILD = 1
POLYMOST = 1
USE_OPENGL = 1
DYNAMIC_OPENGL = 1
USE_A_C = 0
NOASM = 0

# Debugging options
RELEASE?=1

# build locations

SRC=source/
RSRC=rsrc/
EROOT=../build/
EINC=$(EROOT)include/
INC=$(SRC)
o=o

ifneq (0,$(RELEASE))
  # debugging disabled
  debug=-fomit-frame-pointer -O1
else
  # debugging enabled
  debug=-ggdb -O0
endif

CC=gcc
CXX=g++
# -Werror-implicit-function-declaration
OURCFLAGS=$(debug) -W -Wall -Wimplicit -Wno-char-subscripts -Wno-unused \
	-funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS -DNOCOPYPROTECT \
	-I$(INC:/=) -I$(EINC:/=) -I$(SRC)jmact -I$(SRC)jaudiolib -I../jfaud/src
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=-lm
#JFAUDLIBS=../jfaud/libjfaud.a ../jfaud/mpadec/libmpadec/libmpadec.a
NASMFLAGS=-s #-g
EXESUFFIX=

include $(EROOT)Makefile.shared

ifeq ($(PLATFORM),LINUX)
	OBJ=obj.nix/
	EOBJ=eobj.nix/
	NASMFLAGS+= -f elf
else
	OBJ=obj/
	EOBJ=eobj/
endif

JMACTOBJ=$(OBJ)util_lib.$o \
	$(OBJ)file_lib.$o \
	$(OBJ)control.$o \
	$(OBJ)keyboard.$o \
	$(OBJ)mouse.$o \
	$(OBJ)mathutil.$o \
	$(OBJ)scriplib.$o

AUDIOLIB_FX_STUB=$(OBJ)audiolib_fxstub.$o
AUDIOLIB_MUSIC_STUB=$(OBJ)audiolib_musicstub.$o
AUDIOLIB_JFAUD=$(OBJ)jfaud_sounds.$o
AUDIOLIB_FX=$(OBJ)mv_mix.$o \
	  $(OBJ)mv_mix16.$o \
	  $(OBJ)mvreverb.$o \
	  $(OBJ)pitch.$o \
	  $(OBJ)multivoc.$o \
	  $(OBJ)ll_man.$o \
	  $(OBJ)fx_man.$o \
	  $(OBJ)dsoundout.$o
AUDIOLIB_MUSIC=$(OBJ)midi.$o \
	  $(OBJ)mpu401.$o \
	  $(OBJ)music.$o

GAMEOBJS=$(OBJ)game.$o \
	$(OBJ)actors.$o \
	$(OBJ)anim.$o \
	$(OBJ)animlib.$o \
	$(OBJ)config.$o \
	$(OBJ)gamedef.$o \
	$(OBJ)gameexec.$o \
	$(OBJ)global.$o \
	$(OBJ)menus.$o \
	$(OBJ)namesdyn.$o \
	$(OBJ)player.$o \
	$(OBJ)premap.$o \
	$(OBJ)savegame.$o \
	$(OBJ)sector.$o \
	$(OBJ)sounds.$o \
	$(OBJ)rts.$o \
	$(OBJ)testcd.$o \
	$(OBJ)osdfuncs.$o \
	$(OBJ)osdcmds.$o \
	$(JMACTOBJ)

EDITOROBJS=$(OBJ)astub.$o

ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include
	NASMFLAGS+= -DUNDERSCORES -f win32
	GAMEOBJS+= $(OBJ)gameres.$o $(OBJ)winbits.$o
	EDITOROBJS+= $(OBJ)buildres.$o
endif

ifeq ($(RENDERTYPE),SDL)
 	OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell sdl-config --cflags))
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC_STUB) $(AUDIOLIB_FX_STUB) $(OBJ)sounds.$o
	#AUDIOLIBOBJ=$(AUDIOLIB_JFAUD)

	ifeq (1,$(HAVE_GTK2))
		OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		GAMEOBJS+= $(OBJ)game_banner.$o
		EDITOROBJS+= $(OBJ)editor_banner.$o
	endif

	GAMEOBJS+= $(OBJ)game_icon.$o
	EDITOROBJS+= $(OBJ)build_icon.$o
endif

ifeq ($(RENDERTYPE),WIN)
	AUDIOLIBOBJ=$(AUDIOLIB_MUSIC) $(AUDIOLIB_FX) $(OBJ)sounds.$o
	#AUDIOLIBOBJ=$(AUDIOLIB_JFAUD)
endif

GAMEOBJS+= $(AUDIOLIBOBJ)
OURCFLAGS+= $(BUILDCFLAGS)
OURCXXFLAGS+= $(BUILDCFLAGS)

.PHONY: clean all engine $(EOBJ)$(ENGINELIB) $(EOBJ)$(EDITORLIB)

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

all: eduke32$(EXESUFFIX) mapster32$(EXESUFFIX)

eduke32$(EXESUFFIX): $(GAMEOBJS) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(JFAUDLIBS) $(LIBS) $(STDCPPLIB) -Wl,-Map=$@.map
	-rm eduke32.sym$(EXESUFFIX)
	cp eduke32$(EXESUFFIX) eduke32.sym$(EXESUFFIX)
	strip eduke32$(EXESUFFIX)
	
mapster32$(EXESUFFIX): $(EDITOROBJS) $(EOBJ)$(EDITORLIB) $(EOBJ)$(ENGINELIB)
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) -Wl,-Map=$@.map
	-rm mapster32.sym$(EXESUFFIX)
	cp mapster32$(EXESUFFIX) mapster32.sym$(EXESUFFIX)
	strip mapster32$(EXESUFFIX)

include Makefile.deps

.PHONY: enginelib editorlib
enginelib editorlib:
	-mkdir $(EOBJ)
	$(MAKE) -C $(EROOT) "OBJ=$(CURDIR)/$(EOBJ)" \
		SUPERBUILD=$(SUPERBUILD) POLYMOST=$(POLYMOST) \
		USE_OPENGL=$(USE_OPENGL) DYNAMIC_OPENGL=$(DYNAMIC_OPENGL) \
		USE_A_C=$(USE_A_C) NOASM=$(NOASM) RELEASE=$(RELEASE) $@
	
$(EOBJ)$(ENGINELIB): enginelib
$(EOBJ)$(EDITORLIB): editorlib

# RULES
$(OBJ)%.$o: $(SRC)%.nasm
	nasm $(NASMFLAGS) $< -o $@
$(OBJ)%.$o: $(SRC)jaudiolib/%.nasm
	nasm $(NASMFLAGS) $< -o $@
 
$(OBJ)%.$o: $(SRC)%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)%.$o: $(SRC)%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)%.$o: $(SRC)jmact/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
$(OBJ)%.$o: $(SRC)jaudiolib/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
 
$(OBJ)%.$o: $(SRC)misc/%.rc
	windres -i $< -o $@
 
$(OBJ)%.$o: $(SRC)util/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
 
$(OBJ)%.$o: $(RSRC)%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1
 
$(OBJ)game_banner.$o: $(RSRC)game_banner.c
$(OBJ)editor_banner.$o: $(RSRC)editor_banner.c
$(RSRC)game_banner.c: $(RSRC)game.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)editor_banner.c: $(RSRC)build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	
clean:
ifeq ($(PLATFORM),DARWIN)
	cd osx && xcodebuild -target All clean
else
	-rm -f $(OBJ)* eduke32$(EXESUFFIX) eduke32.sym$(EXESUFFIX) mapster32$(EXESUFFIX) mapster32.sym$(EXESUFFIX) core*
endif

veryclean: clean
ifeq ($(PLATFORM),DARWIN)
else
	-rm -f $(EOBJ)*
endif
