#
# Build Makefile for GNU Make
#

include ../Makefile.common


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

OURCOMMONFLAGS=$(BASECOMMONFLAGS) -I$(INC)
OURCFLAGS=$(OURCOMMONFLAGS) $(BASECFLAGS)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
OURASFLAGS=$(BASEASFLAGS)
PRINTLDFLAGS=$(BASELDFLAGS)
OURLDFLAGS=$(OURCOMMONFLAGS) $(PRINTLDFLAGS)

ifneq ($(LUNATIC),0)
    OURCOMMONFLAGS+= -DLUNATIC
endif

UTILLIBS= -lm # -lpthread
UTILADDOBJS=

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
	$(OBJ)/texcache.$o \
	$(OBJ)/dxtfilter.$o \
	$(OBJ)/hightile.$o \
	$(OBJ)/textfont.$o \
	$(OBJ)/smalltextfont.$o \
	$(OBJ)/kplib.$o \
	$(OBJ)/quicklz.$o \
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

ifeq (1,$(LUNATIC))
    ENGINEOBJS+=  $(OBJ)/lunatic.$o
endif

EDITOROBJS=$(OBJ)/build.$o \
	$(OBJ)/config.$o \
	$(OBJ)/defs.$o

# PLATFORM SPECIFIC SETTINGS

ifeq ($(SUBPLATFORM),LINUX)
    OURASFLAGS+= -f elf
endif
ifeq ($(PLATFORM),DARWIN)
    ENGINEOBJS += $(OBJ)/osxbits.$o
    ifneq (0,$(OSX_STARTUPWINDOW))
        ENGINEOBJS += $(OBJ)/startosx.editor.$o
    endif
    OURASFLAGS     += -f macho -DUNDERSCORES
    OURCOMMONFLAGS   += -fno-pic -DUNDERSCORES
endif
ifeq ($(PLATFORM),BSD)
    OURASFLAGS+= -f elf
    OURCOMMONFLAGS+= -I/usr/X11R6/include
endif
ifeq ($(PLATFORM),WINDOWS)
    OURCOMMONFLAGS+= -DUNDERSCORES # -I$(ENETROOT)/include
    OURASFLAGS+= -DUNDERSCORES -f win32
    ENGINEOBJS+= $(OBJ)/winbits.$o
    EDITOROBJS+= $(OBJ)/startwin.editor.$o
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

    ifeq ($(PLATFORM),WINDOWS)
        OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
    endif

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
            OURCOMMONFLAGS += -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                    -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers
        else
            OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
        endif
    endif

    ifeq (1,$(HAVE_GTK2))
        OURCOMMONFLAGS+= -DHAVE_GTK2 $(shell pkg-config --cflags gtk+-2.0)
        ENGINEOBJS+= $(OBJ)/gtkbits.$o
        ifeq ($(LINKED_GTK),0)
            ENGINEOBJS+= $(OBJ)/dynamicgtk.$o
        endif
        EDITOROBJS+= $(OBJ)/startgtk.editor.$o
    endif
endif
ifeq ($(RENDERTYPE),WIN)
    OURCOMMONFLAGS+= -I$(DXROOT) -I$(DXROOT)/include
    ENGINEOBJS+= $(OBJ)/winlayer.$o $(OBJ)/rawinput.$o
endif

OURCOMMONFLAGS+= $(BUILDCOMMONFLAGS)

COMPILER=$(CC) $(OURCONLYFLAGS)
LINKER=$(L_CC)
ifneq ($(CPLUSPLUS),0)
    COMPILER=$(CXX) $(OURCXXFLAGS)
    LINKER=$(L_CXX)
endif

.SILENT:
.PHONY: clean cleanutils veryclean all utils dxutils sdlutils printutils printsdlutils printdxutils enginelib editorlib

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
utils: start $(UTILS) finish
	@ls -l $(UTILS)

dxutils: start $(DXUTILS) finish
	@ls -l $(DXUTILS)

sdlutils: start $(SDLUTILS) finish
	@ls -l $(SDLUTILS)

start:
	$(BUILD_STARTED)

finish:
	$(BUILD_FINISHED)

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
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
kgroup$(EXESUFFIX): $(OBJ)/kgroup.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
transpal$(EXESUFFIX): $(OBJ)/transpal.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
wad2art$(EXESUFFIX): $(OBJ)/wad2art.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
wad2map$(EXESUFFIX): $(OBJ)/wad2map.$o $(OBJ)/pragmas.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
kmd2tool$(EXESUFFIX): $(OBJ)/kmd2tool.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
md2tool$(EXESUFFIX): $(OBJ)/md2tool.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
generateicon$(EXESUFFIX): $(OBJ)/generateicon.$o $(OBJ)/compat.$o $(OBJ)/pragmas.$o $(OBJ)/kplib.$o \
        $(OBJ)/cache1d.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
cacheinfo$(EXESUFFIX): $(OBJ)/cacheinfo.$o $(OBJ)/compat.$o $(OBJ)/compat_tools.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
enumdisplay$(EXESUFFIX): $(OBJ)/enumdisplay.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS) -I$(DXROOT) -I$(DXROOT)/include -lgdi32; then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
getdxdidf$(EXESUFFIX): $(OBJ)/getdxdidf.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS) -I$(DXROOT) -I$(DXROOT)/include -ldinput; then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
makesdlkeytrans$(EXESUFFIX): $(OBJ)/makesdlkeytrans.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS) -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL; then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
arttool$(EXESUFFIX): $(OBJ)/arttool.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(L_CXX) -o $@ $^ $(OURLDFLAGS) $(STATICSTDCPP) $(STDCPPLIB) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
givedepth$(EXESUFFIX): $(OBJ)/givedepth.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
mkpalette$(EXESUFFIX): $(OBJ)/mkpalette.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
unpackssi$(EXESUFFIX): $(OBJ)/unpackssi.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi
bsuite$(EXESUFFIX): $(OBJ)/bsuite.$o $(UTILADDOBJS)
	$(ONESTEP_STATUS)
	if $(LINKER) -o $@ $^ $(OURLDFLAGS) $(UTILLIBS); then $(ONESTEP_OK); else $(ONESTEP_FAILED); fi

# DEPENDENCIES
include Makefile.deps

# RULES
$(OBJ)/%.$o: $(SRC)/%.nasm
	$(COMPILE_STATUS)
	if $(AS) $(OURASFLAGS) $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

# Comment out the following rule to debug a-c.o
$(OBJ)/a-c.$o: $(SRC)/a-c.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(subst -O$(OPTLEVEL),-O2,$(subst -fsanitize=address -fsanitize=undefined,,$(OURCFLAGS))) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi
# KEEPINSYNC the Clang -f option(s) with Makefile.common

$(OBJ)/%.$o: $(SRC)/%.c
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

$(OBJ)/%.$o: $(SRC)/misc/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/misc/%.rc
	$(COMPILE_STATUS)
	if $(RC) -i $< -o $@ --include-dir=$(INC) --include-dir=$(SRC); then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cc
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cpp
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(SRC)/util/%.cxx
	$(COMPILE_STATUS)
	if $(CXX) $(OURCXXFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/%.$o: $(RSRC)/%.c
	$(COMPILE_STATUS)
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

libcache1d.so: $(SRC)/cache1d.c
	$(COMPILE_STATUS)
	if $(COMPILER) -Wall -Wextra -DCACHE1D_COMPRESS_ONLY -shared $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

$(OBJ)/editor_banner.$o: $(RSRC)/editor_banner.c
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@
$(RSRC)/editor_banner.c: $(RSRC)/build.bmp
	echo "#include <gdk-pixbuf/gdk-pixdata.h>" > $@
	echo "extern const GdkPixdata startbanner_pixdata;" >> $@
	gdk-pixbuf-csource --extern --struct --raw --name=startbanner_pixdata $^ | sed 's/load_inc//' >> $@

# PHONIES	

clean:
	-rm -f $(OBJ)/*
	echo -n "" > $(OBJ)/keep.me

cleanutils:
	-rm -f $(UTILS) $(UTILOBJS) $(DXUTILS) $(DXUTILOBJS) $(SDLUTILS) $(SDLUTILOBJS) $(UTILADDOBJS)

veryclean: clean cleanutils
	-rm -f $(ENGINELIB) $(EDITORLIB)

printutils:
	echo "$(UTILS)"

printdxutils:
	echo "$(DXUTILS)"

printsdlutils:
	echo "$(SDLUTILS)"

.PHONY: fixlineends
fixlineends:
	for a in `find . -type f \( -name '*.c' -o -name '*.h' -o -name 'Makefile*' \) \! -path '*/.svn/*'`; do \
		echo Fixing $$a && tr -d "\015" < $$a > $$a.fix && mv $$a.fix $$a; \
	done

# DO NOT DELETE


