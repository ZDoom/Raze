CC=gcc
OBJ=obj
OBJNAME?=libenet.a
PRETTY_OUTPUT?=1
EROOT?=build
RELEASE?=1
OPTLEVEL?=2
SRC=src

include ../../$(EROOT)/Makefile.shared

ifneq (0,$(RELEASE))
	# Debugging disabled
  debug=-fomit-frame-pointer -funswitch-loops -O$(OPTLEVEL)
else
	# Debugging enabled
  debug=-ggdb -O0 -DDEBUGGINGAIDS
endif

ifneq (0,$(DEBUGANYWAY))
  debug+=-ggdb
endif

CFLAGS=$(debug) -W -Wall -Wimplicit -Werror-implicit-function-declaration \
	-funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS -D_FORTIFY_SOURCE=2 \
    -fjump-tables -fno-stack-protector

CPPFLAGS=-Iinclude -Isrc -DHAVE_VORBIS

OBJECTS=$(OBJ)/callbacks.o \
	$(OBJ)/host.o \
	$(OBJ)/list.o \
	$(OBJ)/packet.o \
	$(OBJ)/peer.o \
	$(OBJ)/protocol.o

ifeq ($(PLATFORM),WINDOWS)
 OBJECTS+= $(OBJ)/win32.o
 OBJNAME=libenet_win32.a
 OBJ=obj_win
else
 OBJECTS+= $(OBJ)/unix.o
endif

# OBJECTS=$(SOURCES:%.c=$(OBJ)/%.o)

$(OBJNAME): $(OBJECTS)
	$(AR) cr $@ $^

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
