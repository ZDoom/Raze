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
OURCOMMONFLAGS=$(BASECOMMONFLAGS) $(BUILDCOMMONFLAGS) -I$(INC) -I$(SRC)
ifneq ($(PLATFORM),WII)
    OURCOMMONFLAGS+= -DHAVE_VORBIS
endif
ifneq ($(RELEASE),0)
    # Debugging disabled
    debug+= $(F_NO_STACK_PROTECTOR)
endif
OURCFLAGS=$(OURCOMMONFLAGS) $(BASECFLAGS)
OURCONLYFLAGS=$(BASECONLYFLAGS)
OURCXXFLAGS=$(BASECXXFLAGS)
PRINTLDFLAGS=$(BASELDFLAGS)
OURASFLAGS=$(BASEASFLAGS)

ifeq (1,$(SDL_FRAMEWORK))
    ifeq ($(PLATFORM),DARWIN)
        APPLE_INCLUDE_DIR:=../../Apple/include
        OURCOMMONFLAGS += -I$(APPLE_INCLUDE_DIR) -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers
    endif
endif

COMPILER=$(CC) $(OURCONLYFLAGS)
ifneq ($(CPLUSPLUS),0)
    COMPILER=$(CXX) $(OURCXXFLAGS)
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
    ifeq ($(RENDERTYPE),WIN)
        OURCOMMONFLAGS+= -DHAVE_DS -I$(DXROOT)/include
        OBJECTS+= $(OBJ)/driver_directsound.o
    endif

    OURCOMMONFLAGS+= -Ithird-party/mingw32/include
    OBJNAME=libjfaudiolib_win32.a
    OBJ=obj_win
endif

ifeq ($(RENDERTYPE),SDL)
    OURCOMMONFLAGS+= -DHAVE_SDL
    ifneq ($(PLATFORM),DARWIN)
        ifneq ($(PLATFORM),WINDOWS)
            OURCOMMONFLAGS+=`pkg-config --cflags vorbis`
        endif
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
	if $(COMPILER) $(OURCFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(OBJNAME)
