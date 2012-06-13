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
endif

include Makefile.shared

OURCFLAGS=$(BASECFLAGS) -Wno-char-subscripts -I$(INC)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
OURASFLAGS=$(BASEASFLAGS)
OURLDFLAGS=$(BASELDFLAGS) $(BUILDLDFLAGS)

ifneq ($(LUNATIC),0)
    OURCFLAGS+= -DLUNATIC_ENABLE
endif

UTILLIBS= -lm # -lpthread
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
    OURASFLAGS+= -f elf
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINEOBJS += $(OBJ)/osxbits.$o
    ifneq (0,$(OSX_STARTUPWINDOW))
        ENGINEOBJS += $(OBJ)/StartupWinController.editor.$o
    endif
    OURASFLAGS     += -f macho -DUNDERSCORES
    OURCFLAGS   += -fno-pic -DUNDERSCORES
endif
ifeq ($(PLATFORM),BSD)
    OURASFLAGS+= -f elf
    OURCFLAGS+= -I/usr/X11R6/include
endif
ifeq ($(PLATFORM),WINDOWS)
    OURCFLAGS+= -DUNDERSCORES -I$(DXROOT)/include -I$(FMODROOTWIN)/inc # -I$(ENETROOT)/include
    OURASFLAGS+= -DUNDERSCORES -f win32
endif
ifeq ($(PLATFORM),BEOS)
    OURASFLAGS+= -f elf
    TARGETOPTS+= -DNOASM
endif
ifeq ($(PLATFORM),SYLLABLE)
    OURASFLAGS+= -f elf
endif

ifeq ($(RENDERTYPE),SDL)
    ENGINEOBJS+= $(OBJ)/sdlayer.$o

    ifeq ($(PLATFORM),DARWIN)
        ifneq ($(findstring x86_64,$(ARCH)),x86_64)
            ifeq (,$(ARCH))
                ifneq ($(findstring x86_64,$(SYSARCH)),x86_64)
                    UTILLIBS += -read_only_relocs suppress
                endif
            else
                UTILLIBS += -read_only_relocs suppress
            endif
        endif

        ifeq (1,$(SDL_FRAMEWORK))
            OURCFLAGS += -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                    -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers
            UTILLIBS += -Wl,-framework,SDL -Wl,-framework,SDL_mixer ../Apple/lib/libSDLmain.a
        else
            OURCFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
            UTILLIBS += -lSDL_mixer
        endif
    endif

    ifeq (0,$(SDL_FRAMEWORK))
        OURCFLAGS+= $(subst -Dmain=SDL_main,,$(shell $(SDLCONFIG) --cflags))
        UTILLIBS+= -lSDL_mixer
    else
        OURCFLAGS   += -DSDL_FRAMEWORK
    endif

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

UTILOBJS=$(OBJ)/kextract.$o $(OBJ)/kgroup.$o $(OBJ)/transpal.$o $(OBJ)/wad2art.$o $(OBJ)/wad2map.$o $(OBJ)/kmd2tool.$o \
    $(OBJ)/md2tool.$o \
    $(OBJ)/generateicon.$o $(OBJ)/cacheinfo.$o $(OBJ)/arttool.$o $(OBJ)/givedepth.$o $(OBJ)/mkpalette.$o \
    $(OBJ)/unpackssi.$o $(OBJ)/bsuite.$o \
    $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(OBJ)/pragmas.$o $(OBJ)/kplib.$o $(OBJ)/cache1d.$o
UTILS=kextract$(EXESUFFIX) kgroup$(EXESUFFIX) transpal$(EXESUFFIX) wad2art$(EXESUFFIX) wad2map$(EXESUFFIX) kmd2tool$(EXESUFFIX) \
    md2tool$(EXESUFFIX) \
    generateicon$(EXESUFFIX) cacheinfo$(EXESUFFIX) arttool$(EXESUFFIX) givedepth$(EXESUFFIX) mkpalette$(EXESUFFIX) \
    unpackssi$(EXESUFFIX) bsuite$(EXESUFFIX)

DXUTILOBJS=$(OBJ)/enumdisplay.$o $(OBJ)/getdxdidf.$o
DXUTILS=enumdisplay$(EXESUFFIX) getdxdidf$(EXESUFFIX)

SDLUTILOBJS=$(OBJ)/makesdlkeytrans.$o
SDLUTILS=makesdlkeytrans$(EXESUFFIX)

# all: $(OBJ)/$(ENGINELIB) $(OBJ)/$(EDITORLIB)
utils: start $(UTILS) finish utilfinish
dxutils: start $(DXUTILS) finish dxutilfinish
sdlutils: start $(SDLUTILS) finish sdlutilfinish

start:
	$(BUILD_STARTED)

finish:
	$(BUILD_FINISHED)

utilfinish:
	@ls -l $(UTILS)

dxutilfinish:
	@ls -l $(DXUTILS)

sdlutilfinish:
	@ls -l $(SDLUTILS)

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
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
kgroup$(EXESUFFIX): $(OBJ)/kgroup.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
transpal$(EXESUFFIX): $(OBJ)/transpal.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
wad2art$(EXESUFFIX): $(OBJ)/wad2art.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
wad2map$(EXESUFFIX): $(OBJ)/wad2map.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
kmd2tool$(EXESUFFIX): $(OBJ)/kmd2tool.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
md2tool$(EXESUFFIX): $(OBJ)/md2tool.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
generateicon$(EXESUFFIX): $(OBJ)/generateicon.$o $(OBJ)/compat.$o $(OBJ)/pragmas.$o $(OBJ)/kplib.$o \
        $(OBJ)/cache1d.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
cacheinfo$(EXESUFFIX): $(OBJ)/cacheinfo.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
enumdisplay$(EXESUFFIX): $(OBJ)/enumdisplay.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS) -I$(DXROOT)/include -lgdi32; then $(ONESTEP_OK); fi
getdxdidf$(EXESUFFIX): $(OBJ)/getdxdidf.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS) -I$(DXROOT)/include -ldinput; then $(ONESTEP_OK); fi
makesdlkeytrans$(EXESUFFIX): $(OBJ)/makesdlkeytrans.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
arttool$(EXESUFFIX): $(OBJ)/arttool.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CXX) -o $@ $^ $(OURLDFLAGS) $(STATICSTDCPP) $(UTILLIBS); then $(ONESTEP_OK); fi
givedepth$(EXESUFFIX): $(OBJ)/givedepth.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
mkpalette$(EXESUFFIX): $(OBJ)/mkpalette.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
unpackssi$(EXESUFFIX): $(OBJ)/unpackssi.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi
bsuite$(EXESUFFIX): $(OBJ)/bsuite.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(CC) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); fi

# DEPENDENCIES
include Makefile.deps

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	if $(AS) $(OURASFLAGS) $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

# uncomment to debug a-c.o
$(OBJ)/a-c.$o: $(SRC)/a-c.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(subst -O0,-O2,$(subst -fcatch-undefined-behavior,,$(OURCFLAGS))) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/%.m
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

$(OBJ)/%.$o: $(SRC)/misc/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(INC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

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
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

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
	-rm -f $(UTILS) $(UTILOBJS) $(DXUTILS) $(DXUTILOBJS) $(SDLUTILS) $(SDLUTILOBJS) $(UTILADDOBJS)

printutils:
	echo "$(UTILS)"

printdxutils:
	echo "$(DXUTILS)"

printsdlutils:
	echo "$(SDLUTILS)"

veryclean: clean cleanutils
	-rm -f $(ENGINELIB) $(EDITORLIB)

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done

# DO NOT DELETE


