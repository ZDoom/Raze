include ../../Makefile.common
include ../../$(EROOT)/Makefile.shared

OBJ=obj
OBJNAME=libjfaudiolib.a
SRC=src
INC=include

# for BUILD_ECHOFLAGS:
OURCOMMONFLAGS=$(BASECOMMONFLAGS) $(BUILDCOMMONFLAGS) -I$(INC) -I$(SRC) -DHAVE_VORBIS
ifneq ($(PLATFORM),WII)
    OURCOMMONFLAGS+= -DHAVE_FLAC
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

ifeq ($(PLATFORM),DARWIN)
    ifeq (1,$(SDL_FRAMEWORK))
        OURCOMMONFLAGS += -I$(APPLE_FRAMEWORKS)/SDL.framework/Headers \
                -I$(APPLE_FRAMEWORKS)/SDL_mixer.framework/Headers
    else
        OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
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
	$(OBJ)/formats.o \
	$(OBJ)/vorbis.o \
	$(OBJ)/flac.o \
	$(OBJ)/driver_nosound.o

ifeq ($(PLATFORM),DARWIN)
    OURCOMMONFLAGS+= -Ithird-party/common/include
endif

ifeq ($(PLATFORM),WINDOWS)
    ifeq ($(RENDERTYPE),WIN)
        OURCOMMONFLAGS+= -DHAVE_DS -I$(DXROOT) -I$(DXROOT)/include
        OBJECTS+= $(OBJ)/driver_directsound.o
    endif

    OURCOMMONFLAGS+= -Ithird-party/common/include
    OBJNAME=libjfaudiolib_win32.a
    OBJ=obj_win
endif

ifeq ($(RENDERTYPE),SDL)
    OURCOMMONFLAGS+= -DHAVE_SDL
    ifeq ($(PLATFORM),WINDOWS)
        OURCOMMONFLAGS += -I$(SDLROOT)/include -I$(SDLROOT)/include/SDL
    endif
    ifneq ($(PLATFORM),DARWIN)
        ifneq ($(PLATFORM),WINDOWS)
            ifneq ($(PLATFORM),WII)
                OURCOMMONFLAGS+=`pkg-config --cflags vorbis`
            endif
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
