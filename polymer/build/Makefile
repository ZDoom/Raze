# Build Makefile for GNU Make

# Notes:
#  As of 6 July 2005, the engine should handle optimisations being enabled.
#  If things seem to be going wrong, lower or disable optimisations, then
#  try again. If things are still going wrong, call me.
#   

# Engine options - these may be overridden by game makefiles
#  SUPERBUILD     - enables voxels
#  POLYMOST       - enables Polymost renderer
#  USE_OPENGL     - enables OpenGL support in Polymost
#  DYNAMIC_OPENGL - enables run-time loading of OpenGL library
#  USE_A_C        - enables use of C version of classic renderer
#  NOASM          - disables the use of inline assembly pragmas
#
#  SETSPRITEZ     - set to 1 for Shadow Warrior
SUPERBUILD ?= 1
POLYMOST ?= 1		
USE_OPENGL ?= 1
DYNAMIC_OPENGL ?= 1
USE_A_C ?= 0
NOASM ?= 0

SETSPRITEZ ?= 0

# Debugging options
#  RELEASE - 1 = no debugging
#  EFENCE  - 1 = compile with Electric Fence for malloc() debugging
RELEASE?=1
EFENCE?=0

# SDK locations - adjust to match your setup
DXROOT=c:/sdks/directx/dx61
FMODROOTWIN=c:/sdks/fmodapi374win/api

# build locations - OBJ gets overridden to the game-specific objects dir
OBJ?=obj.gnu/
SRC=src/
RSRC=rsrc/
INC=include/

# filename extensions - these won't need to change
o=o
res=o
asm=nasm

# debugging and release
ifneq ($(RELEASE),0)
  # debugging disabled
  debug=-fomit-frame-pointer -O2
else
  # debugging enabled
  debug=-ggdb -O0 -DDEBUGGINGAIDS -DNOSDLPARACHUTE
endif

CC=gcc
CXX=gcc
AS=nasm
RC=windres
AR=ar
RANLIB=ranlib
OURCFLAGS=$(debug) -W -Wall -Wimplicit -Wno-char-subscripts -Wno-unused \
	-funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS \
	-DKSFORBUILD -I$(INC:/=) -I../jfaud/src
OURCXXFLAGS=-fno-exceptions -fno-rtti
LIBS=
GAMELIBS=../jfaud/libjfaud.a #../jfaud/mpadec/libmpadec/libmpadec.a
ASFLAGS=-s #-g
EXESUFFIX=

include Makefile.shared

ifneq (0,$(USE_A_C))
  ENGINEOBJS=$(OBJ)a-c.$o
else
  ENGINEOBJS=$(OBJ)a.$o
endif

ENGINEOBJS+= \
	$(OBJ)baselayer.$o \
	$(OBJ)cache1d.$o \
	$(OBJ)compat.$o \
	$(OBJ)crc32.$o \
	$(OBJ)defs.$o \
	$(OBJ)engine.$o \
	$(OBJ)engineinfo.$o \
	$(OBJ)glbuild.$o \
	$(OBJ)kplib.$o \
	$(OBJ)lzf_c.$o \
	$(OBJ)lzf_d.$o \
	$(OBJ)md4.$o \
	$(OBJ)mmulti.$o \
	$(OBJ)osd.$o \
	$(OBJ)pragmas.$o \
	$(OBJ)scriptfile.$o

EDITOROBJS=$(OBJ)build.$o \
	$(OBJ)config.$o

GAMEEXEOBJS=$(OBJ)game.$o \
	$(OBJ)jfaud_sound.$o \
	$(OBJ)config.$o \
	$(OBJ)$(ENGINELIB)

EDITOREXEOBJS=$(OBJ)bstub.$o \
	$(OBJ)$(EDITORLIB) \
	$(OBJ)$(ENGINELIB)

# detect the platform
ifeq ($(PLATFORM),LINUX)
	ASFLAGS+= -f elf
	LIBS+= -lm
endif
ifeq ($(PLATFORM),BSD)
	ASFLAGS+= -f elf
	OURCFLAGS+= -I/usr/X11R6/include
	LIBS+= -lm
endif
ifeq ($(PLATFORM),WINDOWS)
	OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include -I$(FMODROOTWIN)/inc
	LIBS+= -lm
	GAMELIBS+= -L$(FMODROOTWIN)/lib
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
	ENGINEOBJS+= $(OBJ)sdlayer.$o
	OURCFLAGS+= $(subst -Dmain=SDL_main,,$(SDLCONFIG_CFLAGS))
	
	ifeq (1,$(HAVE_GTK2))
		OURCFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
		ENGINEOBJS+= $(OBJ)gtkbits.$o
		GAMEEXEOBJS+= $(OBJ)game_banner.$o
		EDITOREXEOBJS+= $(OBJ)editor_banner.$o
	endif

	GAMEEXEOBJS+= $(OBJ)game_icon.$o
	EDITOREXEOBJS+= $(OBJ)build_icon.$o
endif
ifeq ($(RENDERTYPE),WIN)
	ENGINEOBJS+= $(OBJ)winlayer.$o
	EDITOROBJS+= $(OBJ)buildstartwin.$o
	GAMEEXEOBJS+= $(OBJ)gameres.$(res) $(OBJ)gamestartwin.$o
	EDITOREXEOBJS+= $(OBJ)buildres.$(res)
endif

ifneq (0,$(EFENCE))
	LIBS+= -lefence
	OURCFLAGS+= -DEFENCE
endif

OURCFLAGS+= $(BUILDCFLAGS)

.PHONY: clean veryclean all utils writeengineinfo enginelib editorlib

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
	cd osx/engine && xcodebuild -target All -buildstyle $(style)
	cd osx/game && xcodebuild -target All -buildstyle $(style)
endif

UTILS=kextract$(EXESUFFIX) kgroup$(EXESUFFIX) transpal$(EXESUFFIX) wad2art$(EXESUFFIX) wad2map$(EXESUFFIX)

all: game$(EXESUFFIX) build$(EXESUFFIX) $(OBJ)$(ENGINELIB) $(OBJ)$(EDITORLIB)
utils: $(UTILS)

enginelib: $(OBJ)$(ENGINELIB)
$(OBJ)$(ENGINELIB): $(ENGINEOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

editorlib: $(OBJ)$(EDITORLIB)
$(OBJ)$(EDITORLIB): $(EDITOROBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

game$(EXESUFFIX): $(GAMEEXEOBJS)
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS) $(GAMELIBS) $(STDCPPLIB)
	
build$(EXESUFFIX): $(EDITOREXEOBJS)
	$(CC) $(CFLAGS) $(OURCFLAGS) -o $@ $^ $(LIBS)

pragmacheck$(EXESUFFIX): $(OBJ)pragmacheck.$o $(OBJ)pragmas.$o
	$(CC) $(subst -Dmain=app_main,,$(OURCFLAGS)) -o $@ $^
	
kextract$(EXESUFFIX): $(OBJ)kextract.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
kgroup$(EXESUFFIX): $(OBJ)kgroup.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
transpal$(EXESUFFIX): $(OBJ)transpal.$o $(OBJ)pragmas.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
wad2art$(EXESUFFIX): $(OBJ)wad2art.$o $(OBJ)pragmas.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
wad2map$(EXESUFFIX): $(OBJ)wad2map.$o $(OBJ)pragmas.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
generateicon$(EXESUFFIX): $(OBJ)generateicon.$o $(OBJ)kplib.$o
	$(CC) -o $@ $^
cacheinfo$(EXESUFFIX): $(OBJ)cacheinfo.$o $(OBJ)compat.$o
	$(CC) -o $@ $^
enumdisplay$(EXESUFFIX): src/misc/enumdisplay.c
	$(CC) -g -Os -o $@ $^ -I$(DXROOT)/include -lgdi32

# DEPENDENCIES
include Makefile.deps

.PHONY: $(OBJ)engineinfo.$o
$(OBJ)engineinfo.$o:
	echo "const char _engine_cflags[] = \"$(CFLAGS) $(OURCFLAGS)\";" > $(SRC)tmp/engineinfo.c
	echo "const char _engine_libs[] = \"$(LIBS)\";" >> $(SRC)tmp/engineinfo.c
	echo "const char _engine_uname[] = \"$(shell uname -a)\";" >> $(SRC)tmp/engineinfo.c
	echo "const char _engine_compiler[] = \"$(CC) $(shell $(CC) -dumpversion) $(shell $(CC) -dumpmachine)\";" >> $(SRC)tmp/engineinfo.c
	echo "const char _engine_date[] = __DATE__ \" \" __TIME__;" >> $(SRC)tmp/engineinfo.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $(SRC)tmp/engineinfo.c -o $@ 2>&1

# RULES
$(OBJ)%.$o: $(SRC)%.nasm
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ)%.$o: $(SRC)%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1

$(OBJ)%.$o: $(SRC)%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1

$(OBJ)%.$o: $(SRC)tmp/%.c
	$(CC) $(CFLAGS) $(OURCFLAGS) -c $< -o $@ 2>&1

$(OBJ)%.$o: $(SRC)misc/%.rc
	$(RC) -i $^ -o $@

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
	cd osx/engine && xcodebuild -target All clean
	cd osx/game && xcodebuild -target All clean
else
	-rm -f $(OBJ)*
endif

veryclean: clean
ifeq ($(PLATFORM),DARWIN)
else
	-rm -f $(ENGINELIB) $(EDITORLIB) game$(EXESUFFIX) build$(EXESUFFIX) $(UTILS)
endif

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done
