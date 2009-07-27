CC=gcc
OBJ=obj
OBJNAME?=libjfaudiolib.a
PRETTY_OUTPUT?=1
EROOT?=build
SRC=src

include ../../$(EROOT)/Makefile.shared

CFLAGS=-g -O2 -Wall
CPPFLAGS=-Iinclude -Isrc -DHAVE_VORBIS

OBJECTS=$(OBJ)/drivers.o \
	$(OBJ)/fx_man.o \
	$(OBJ)/multivoc.o \
	$(OBJ)/mix.o \
	$(OBJ)/mixst.o \
	$(OBJ)/pitch.o \
	$(OBJ)/vorbis.o \
	$(OBJ)/driver_nosound.o

ifeq ($(PLATFORM),WINDOWS)
 CPPFLAGS+= -I/z/sdks/directx/dx7/include -Ithird-party/mingw32/include
 OBJECTS+= $(OBJ)/driver_directsound.o
 OBJNAME=libjfaudiolib_win32.a
 OBJ=obj_win
else
 CPPFLAGS+= -DHAVE_SDL
 OBJECTS+= $(OBJ)/driver_sdl.o
endif

# OBJECTS=$(SOURCES:%.c=$(OBJ)/%.o)

$(OBJNAME): $(OBJECTS)
	ar cr $@ $^

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
