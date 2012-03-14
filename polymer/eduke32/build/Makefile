#
# Build Makefile for GNU Make
#

# Notes:
#  As of 6 July 2005, the engine should handle optimisations being enabled.
#  If things seem to be going wrong, lower or disable optimisations, then
#  try again. If things are still going wrong, call me.
#   

include ../Makefile.common


# SDK locations for Windows - adjust to match your setup
#
ifneq ($(DXROOT_OVERRIDE),)
    DXROOT=$(DXROOT_OVERRIDE)
else
    DXROOT=../../sdk/dx
    #DXROOT=c:/sdks/directx/dx8
endif
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
    debug+= $(F_NO_STACK_PROTECTOR)
else
    # Debugging enabled
    debug+= -DNOSDLPARACHUTE
    LIBS+= $(L_SSP) -Wl,--enable-auto-import
endif

OURCFLAGS=$(BASECFLAGS) -Wno-char-subscripts -I$(INC) $(ARCH)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
ASFLAGS=$(BASEASFLAGS)

ifneq ($(LUNATIC),0)
    OURCFLAGS+= -DLUNATIC_ENABLE
endif

include Makefile.shared

UTILLIBS=  # -lpthread
UTILADDOBJS=
ifneq (0,$(NEDMALLOC))
    UTILADDOBJS+= $(OBJ)/nedmalloc.$o
endif

ENGINEOBJS=
ifeq (0,$(NOASM))
  ENGINEOBJS+= $(OBJ)/a.$o
else
  ENGINEOBJS+= $(OBJ)/a-c.$o
endif

ifneq ($(PLATFORM),WINDOWS)
    LIBS+= -ldl -pthread
endif
ifeq ($(PLATFORM),DARWIN)
    ifneq ($(USE_LIBPNG),0)
        OURCFLAGS+= -I/opt/local/include
        LIBS+= -L/opt/local/lib -lpng
    endif
endif


ENGINEOBJS+= \
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
	$(OBJ)/mutex.$o

ifeq (1,$(USE_OPENGL))
    ENGINEOBJS+=  $(OBJ)/mdsprite.$o
    ENGINEOBJS+=  $(OBJ)/glbuild.$o
endif

ifeq (1,$(POLYMER))
    ENGINEOBJS+=  $(OBJ)/polymer.$o
endif

ifeq (1,$(NEDMALLOC))
    ENGINEOBJS+=  $(OBJ)/nedmalloc.$o
endif

EDITOROBJS=$(OBJ)/build.$o \
	$(OBJ)/config.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(PLATFORM),LINUX)
    ASFLAGS+= -f elf
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINEOBJS += $(OBJ)/osxbits.$o
    ifneq (0,$(OSX_STARTUPWINDOW))
        ENGINEOBJS += $(OBJ)/StartupWinController.editor.$o
    endif
    ASFLAGS     += -f macho -DUNDERSCORES
    OURCFLAGS   += -fno-pic -DUNDERSCORES
endif
ifeq ($(PLATFORM),BSD)
    ASFLAGS+= -f elf
    OURCFLAGS+= -I/usr/X11R6/include
endif
ifeq ($(PLATFORM),WINDOWS)
    OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include -I$(FMODROOTWIN)/inc # -I$(ENETROOT)/include
    LIBS+= -lwsock32 -lws2_32 -lwinmm #-L$(ENETROOT)
    ASFLAGS+= -DUNDERSCORES -f win32
endif
ifeq ($(PLATFORM),BEOS)
    ASFLAGS+= -f elf
    TARGETOPTS+= -DNOASM
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

OURCFLAGS+= $(BUILDCFLAGS)

.SILENT:
.PHONY: clean cleanutils veryclean all utils enginelib editorlib

# TARGETS

UTILOBJS=$(OBJ)/kextract.$o $(OBJ)/kgroup.$o $(OBJ)/transpal.$o $(OBJ)/wad2art.$o $(OBJ)/wad2map.$o $(OBJ)/md2tool.$o \
    $(OBJ)/generateicon.$o $(OBJ)/cacheinfo.$o $(OBJ)/enumdisplay.$o $(OBJ)/arttool.$o $(OBJ)/givedepth.$o $(OBJ)/mkpalette.$o \
    $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(OBJ)/pragmas.$o $(OBJ)/kplib.$o $(OBJ)/cache1d.$o
UTILS=kextract$(EXESUFFIX) kgroup$(EXESUFFIX) transpal$(EXESUFFIX) wad2art$(EXESUFFIX) wad2map$(EXESUFFIX) md2tool$(EXESUFFIX) \
    generateicon$(EXESUFFIX) cacheinfo$(EXESUFFIX) arttool$(EXESUFFIX) givedepth$(EXESUFFIX) mkpalette$(EXESUFFIX)

# all: $(OBJ)/$(ENGINELIB) $(OBJ)/$(EDITORLIB)
utils: start $(UTILS) finish

start:
	$(BUILD_STARTED)

finish:
	$(BUILD_FINISHED)
	@ls -l $(UTILS)

enginelib: $(OBJ)/$(ENGINELIB)
$(OBJ)/$(ENGINELIB): $(ENGINEOBJS)
	$(BUILD_ECHOFLAGS)
	$(ARCHIVE_STATUS)
	if $(AR) rc $@ $^; then $(ARCHIVE_OK); else $(ARCHIVE_FAILED); fi
	$(RANLIB) $@

editorlib: $(OBJ)/$(EDITORLIB)
$(OBJ)/$(EDITORLIB): $(EDITOROBJS)
	$(BUILD_ECHOFLAGS)
	$(ARCHIVE_STATUS)
	if $(AR) rc $@ $^; then $(ARCHIVE_OK); else $(ARCHIVE_FAILED); fi
	$(RANLIB) $@

#pragmacheck$(EXESUFFIX): $(OBJ)/pragmacheck.$o $(OBJ)/pragmas.$o
#	$(CC) $(subst -Dmain=app_main,,$(OURCFLAGS)) -o $@ $^

kextract$(EXESUFFIX): $(OBJ)/kextract.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
kgroup$(EXESUFFIX): $(OBJ)/kgroup.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
transpal$(EXESUFFIX): $(OBJ)/transpal.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
wad2art$(EXESUFFIX): $(OBJ)/wad2art.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
wad2map$(EXESUFFIX): $(OBJ)/wad2map.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
md2tool$(EXESUFFIX): $(OBJ)/md2tool.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
generateicon$(EXESUFFIX): $(OBJ)/generateicon.$o $(OBJ)/compat.$o $(OBJ)/pragmas.$o $(OBJ)/kplib.$o \
        $(OBJ)/cache1d.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
cacheinfo$(EXESUFFIX): $(OBJ)/cacheinfo.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
enumdisplay$(EXESUFFIX): $(OBJ)/enumdisplay.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS) -I$(DXROOT)/include -lgdi32; then $(ONESTEP_OK); fi
arttool$(EXESUFFIX): $(OBJ)/arttool.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CXX) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
givedepth$(EXESUFFIX): $(OBJ)/givedepth.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi
mkpalette$(EXESUFFIX): $(OBJ)/mkpalette.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(UTILLIBS); then $(ONESTEP_OK); fi

# DEPENDENCIES
include Makefile.deps

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	if $(AS) $(ASFLAGS) $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.m
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cc
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.cxx
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(INC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cc
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cxx
	$(COMPILE_STATUS)
	if $(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(CFLAGS) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/*
	echo -n "" > $(OBJ)/keep.me

cleanutils:
	-rm -f $(UTILS) $(UTILOBJS) $(UTILADDOBJS)

veryclean: clean cleanutils
	-rm -f $(ENGINELIB) $(EDITORLIB)

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done

# DO NOT DELETE


