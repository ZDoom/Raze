include ../../Makefile.common
include ../../$(EROOT)/Makefile.shared

OBJ=obj
OBJNAME=libjfaudiolib.a
SRC=src
INC=include

# SDK locations - adjust to match your setup
ifneq ($(DXROOT_OVERRIDE),)
    DXROOT ?= $(DXROOT_OVERRIDE)
else
    DXROOT=../../../sdk/dx
    #DXROOT ?= c:/sdks/directx/dx8
endif

# for BUILD_ECHOFLAGS:
OURCFLAGS=$(BASECFLAGS) $(BUILDCFLAGS) -I$(INC) -I$(SRC)
ifneq ($(PLATFORM),WII)
    OURCFLAGS+= -DHAVE_VORBIS
endif
ifneq ($(RELEASE),0)
    # Debugging disabled
    debug+= $(F_NO_STACK_PROTECTOR)
endif
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
OURLDFLAGS=$(BASELDFLAGS)
OURASFLAGS=$(BASEASFLAGS)

ifeq ($(PLATFORM),DARWIN)
    ifeq (1,$(SDL_FRAMEWORK))
        APPLE_INCLUDE_DIR:=../../Apple/include
        OURCFLAGS += -DSDL_FRAMEWORK -I$(APPLE_INCLUDE_DIR) -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers
    endif
endif


OBJECTS=$(OBJ)/drivers.o \
	$(OBJ)/fx_man.o \
	$(OBJ)/multivoc.o \
	$(OBJ)/mix.o \
	$(OBJ)/mixst.o \
	$(OBJ)/pitch.o \
	$(OBJ)/vorbis.o \
	$(OBJ)/driver_nosound.o

ifeq ($(PLATFORM),WINDOWS)
    OURCFLAGS+= -I$(DXROOT)/include -Ithird-party/mingw32/include
    OBJECTS+= $(OBJ)/driver_directsound.o
    OBJNAME=libjfaudiolib_win32.a
    OBJ=obj_win
else
    OURCFLAGS+= -DHAVE_SDL $(SDLCONFIG_CFLAGS)
    ifneq ($(PLATFORM),DARWIN)
        OURCFLAGS+=`pkg-config --cflags vorbis`
    endif
    OBJECTS+= $(OBJ)/driver_sdl.o
endif

# OBJECTS=$(SOURCES:%.c=$(OBJ)/%.o)

$(OBJNAME): $(OBJECTS)
	$(BUILD_ECHOFLAGS)
	$(ARCHIVE_STATUS)
	if $(AR) cr $@ $^; then $(ARCHIVE_OK); else $(ARCHIVE_FAILED); fi

$(OBJECTS): $(OBJ)/%.o: $(SRC)/%.c
	-mkdir -p $(OBJ)
	$(COMPILE_STATUS)
	if $(CC) $(OURCONLYFLAGS) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(OBJNAME)
