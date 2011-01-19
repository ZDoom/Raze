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
    DXROOT ?= c:/sdks/directx/dx8
endif

CFLAGS=$(BASECFLAGS) $(F_NO_STACK_PROTECTOR)
# for BUILD_ECHOFLAGS:
OURCFLAGS=$(CFLAGS)
CPPFLAGS=-I$(INC) -I$(SRC) -DHAVE_VORBIS


OBJECTS=$(OBJ)/drivers.o \
	$(OBJ)/fx_man.o \
	$(OBJ)/multivoc.o \
	$(OBJ)/mix.o \
	$(OBJ)/mixst.o \
	$(OBJ)/pitch.o \
	$(OBJ)/vorbis.o \
	$(OBJ)/driver_nosound.o

ifeq ($(PLATFORM),WINDOWS)
    CPPFLAGS+= -I$(DXROOT)/include -Ithird-party/mingw32/include
    OBJECTS+= $(OBJ)/driver_directsound.o
    OBJNAME=libjfaudiolib_win32.a
    OBJ=obj_win
else
    CPPFLAGS+= -DHAVE_SDL
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
	if $(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@; then $(COMPILE_OK); else $(COMPILE_FAILED); fi

ifeq ($(PRETTY_OUTPUT),1)
.SILENT:
endif
.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(OBJNAME)
