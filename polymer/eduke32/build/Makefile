#
# Build Makefile for GNU Make
#

# Notes:
#  As of 6 July 2005, the engine should handle optimisations being enabled.
#  If things seem to be going wrong, lower or disable optimisations, then
#  try again. If things are still going wrong, call me.
#   

# Use colored output
PRETTY_OUTPUT = 1

# Engine options - these may be overridden by game makefiles
#  SUPERBUILD     - enables voxels
#  POLYMOST       - enables Polymost renderer
#  USE_OPENGL     - enables OpenGL support in Polymost
#  NOASM          - disables the use of inline assembly pragmas
#  LINKED_GTK     - enables compile-time linkage to GTK
#

SUPERBUILD ?= 1
POLYMOST ?= 1		
POLYMER ?= 1
USE_OPENGL ?= 1
NOASM ?= 0
LINKED_GTK ?= 0
BUILD32_ON_64 ?= 0

ifeq (0,$(USE_OPENGL))
	POLYMER = 0
endif

# Debugging/Build options
#  RELEASE - 1 = no debugging
#  EFENCE  - 1 = compile with Electric Fence for malloc() debugging
#  OPTLEVEL	- 0..3 = GCC optimization strategy 
#
RELEASE?=1
DEBUGANYWAY?=0
KRANDDEBUG?=0
EFENCE?=0
OPTLEVEL	?= 2

ifneq (0,$(KRANDDEBUG))
    RELEASE=0
endif

# SDK locations for Windows - adjust to match your setup
#
DXROOT=c:/sdks/directx/dx61
FMODROOTWIN=c:/sdks/fmodapi374win/api

# Build locations - OBJ gets overridden to the game-specific objects dir
#
OBJ?=obj.gnu
SRC=src
RSRC=rsrc
INC=include

# ENETROOT=$(SRC)/enet

# Filename extensions - these won't need to change
#
o=o
res=o
asm=nasm

# Debugging and release
#
ifneq ($(RELEASE),0)
  # Debugging disabled
  debug=-fomit-frame-pointer -funswitch-loops -O$(OPTLEVEL)
else
  # Debugging enabled
  debug=-ggdb -O0 -DDEBUGGINGAIDS -DNOSDLPARACHUTE
  ifneq (0,$(KRANDDEBUG))
    debug+=-DKRANDDEBUG=1
  endif
endif

ifneq (0,$(DEBUGANYWAY))
  debug+=-ggdb
endif

CC=gcc
CXX=gcc
AS=nasm
RC=windres
AR=ar
RANLIB=ranlib

GCC_MAJOR    := $(shell $(CC) -dumpversion 2>&1 | cut -d'.' -f1)
GCC_MINOR    := $(shell $(CC) -dumpversion 2>&1 | cut -d'.' -f2)

ifeq (4,$(GCC_MAJOR))
#  debug += -ftree-vectorize
  ifneq (0,$(GCC_MINOR))
    ifneq (1,$(GCC_MINOR))
      ifneq (2,$(GCC_MINOR))
        # this should only enable for 4.3.0 and later
#        debug += -finline-small-functions -fpredictive-commoning
      endif
    endif
    ifeq (5,$(GCC_MINOR))
#	debug += -flto
    endif
  endif
endif

OURCFLAGS=$(debug) -W -Wall -Wimplicit -Werror-implicit-function-declaration \
	-Wno-char-subscripts -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-DKSFORBUILD -I$(INC) -D_FORTIFY_SOURCE=2 \
	-fjump-tables -fno-stack-protector
#    -march=pentium3 -mtune=generic -mmmx -m3dnow -msse -mfpmath=sse
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=
ASFLAGS=-s #-g
EXESUFFIX=

include Makefile.shared

ENGINEOBJS=
ifeq (0,$(NOASM))
  ENGINEOBJS+= $(OBJ)/a.$o
endif

ifneq ($(PLATFORM),WINDOWS)
    LIBS+= -ldl -pthread
endif


ENGINEOBJS+= \
	$(OBJ)/a-c.$o \
	$(OBJ)/baselayer.$o \
	$(OBJ)/cache1d.$o \
	$(OBJ)/compat.$o \
	$(OBJ)/crc32.$o \
	$(OBJ)/defs.$o \
	$(OBJ)/engine.$o \
	$(OBJ)/polymost.$o \
	$(OBJ)/hightile.$o \
	$(OBJ)/textfont.$o \
	$(OBJ)/smalltextfont.$o \
	$(OBJ)/kplib.$o \
	$(OBJ)/quicklz.$o \
	$(OBJ)/md4.$o \
	$(OBJ)/osd.$o \
	$(OBJ)/pragmas.$o \
	$(OBJ)/scriptfile.$o \
    $(OBJ)/nedmalloc.$o \
    $(OBJ)/mutex.$o

ifeq (1,$(USE_OPENGL))
	ENGINEOBJS+=  $(OBJ)/mdsprite.$o
	ENGINEOBJS+=  $(OBJ)/glbuild.$o
endif

ifeq (1,$(POLYMER))
	ENGINEOBJS+=  $(OBJ)/polymer.$o
endif

EDITOROBJS=$(OBJ)/build.$o \
	$(OBJ)/config.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
	ASFLAGS+= -f elf
	LIBS+= -lm 
endif
ifeq ($(PLATFORM),DARWIN)
	ENGINEOBJS	+= $(OBJ)/StartupWinController.editor.$o \
			$(OBJ)//osxbits.$o
	ASFLAGS		+= -f Mach-O
	OURCFLAGS	+= -fno-pic
	TARGETOPTS	+= -DNOASM
endif
ifeq ($(PLATFORM),BSD)
	ASFLAGS+= -f elf
	OURCFLAGS+= -I/usr/X11R6/include
	LIBS+= -lm
endif
ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include -I$(FMODROOTWIN)/inc # -I$(ENETROOT)/include
	LIBS+= -lm -lwsock32 -lws2_32 -lwinmm #-L$(ENETROOT)
	ASFLAGS+= -DUNDERSCORES -f win32
endif
ifeq ($(PLATFORM),BEOS)
	ASFLAGS+= -f elf
	TARGETOPTS+= -DNOASM
endif
ifeq ($(PLATFORM),SUNOS)
	LIBS+= -lm
endif
ifeq ($(PLATFORM),SYLLABLE)
	ASFLAGS+= -f elf
endif

ifeq ($(RENDERTYPE),SDL)
	ENGINEOBJS+= $(OBJ)/sdlayer.$o
	OURCFLAGS+= $(subst -Dmain=SDL_main,,$(SDLCONFIG_CFLAGS))
	
	ifeq (1,$(HAVE_GTK2))
		OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		ENGINEOBJS+= $(OBJ)/gtkbits.$o
		ifeq ($(LINKED_GTK),0)
			ENGINEOBJS+= $(OBJ)/dynamicgtk.$o
		endif
		EDITOROBJS+= $(OBJ)/startgtk.editor.$o
	endif
endif
ifeq ($(RENDERTYPE),WIN)
	ENGINEOBJS+= $(OBJ)/winlayer.$o $(OBJ)/rawinput.$o
	EDITOROBJS+= $(OBJ)/startwin.editor.$o
endif

ifneq (0,$(EFENCE))
	LIBS+= -lefence
	OURCFLAGS+= -DEFENCE
endif

OURCFLAGS+= $(BUILDCFLAGS)

.SILENT:
.PHONY: clean veryclean all utils enginelib editorlib

# TARGETS

UTILS=kextract$(EXESUFFIX) kgroup$(EXESUFFIX) transpal$(EXESUFFIX) wad2art$(EXESUFFIX) wad2map$(EXESUFFIX)

# all: $(OBJ)/$(ENGINELIB) $(OBJ)/$(EDITORLIB)
utils: $(UTILS)

enginelib: $(OBJ)/$(ENGINELIB)
$(OBJ)/$(ENGINELIB): $(ENGINEOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

editorlib: $(OBJ)/$(EDITORLIB)
$(OBJ)/$(EDITORLIB): $(EDITOROBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

pragmacheck$(EXESUFFIX): $(OBJ)/pragmacheck.$o $(OBJ)/pragmas.$o
	$(CC) $(subst -Dmain=app_main,,$(OURCFLAGS)) -o $@ $^
	
kextract$(EXESUFFIX): $(OBJ)/kextract.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
kgroup$(EXESUFFIX): $(OBJ)/kgroup.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
transpal$(EXESUFFIX): $(OBJ)/transpal.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
wad2art$(EXESUFFIX): $(OBJ)/wad2art.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
wad2map$(EXESUFFIX): $(OBJ)/wad2map.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
generateicon$(EXESUFFIX): $(OBJ)/generateicon.$o $(OBJ)/kplib.$o
	$(CC) -o $@ $^ $(LIBS)
cacheinfo$(EXESUFFIX): $(OBJ)/cacheinfo.$o $(OBJ)/compat.$o $(OBJ)/nedmalloc.$o
	$(CC) -o $@ $^ $(LIBS)
enumdisplay$(EXESUFFIX): src/misc/enumdisplay.c
	$(CC) -g -Os -o $@ $^ $(LIBS) -I$(DXROOT)/include -lgdi32
mapdump$(EXESUFFIX): $(OBJ)/mapdump.$o
	$(CC) -o $@ $^ $(LIBS)

# DEPENDENCIES
include Makefile.deps

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	if $(AS) $(ASFLAGS) $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.m
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cxx
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(INC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/*

veryclean: clean
	-rm -f $(ENGINELIB) $(EDITORLIB) $(UTILS)

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done

# DO NOT DELETE


