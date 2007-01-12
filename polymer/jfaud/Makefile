# OpenAL
# http://www.openal.org/
USEAL=1 
LINKAL=0
ALCFLAGS=
ALLIBS=-lopenal
ALDLWIN=openal32.dll
ALDLLIN=libopenal.so

# DirectX SDK (Win32)
# http://www.microsoft.com/downloads/details.aspx?FamilyID=edb98ffa-a59c-4c23-9b92-ba304f188314&DisplayLang=en
DIRECTXCFLAGS=
DIRECTXLIBS=-ldxguid

# OggVorbis
# http://www.vorbis.com/
USEVORBIS=1
LINKVORBIS=0
VORBISCFLAGS=
VORBISLIBS=-lvorbisfile -lvorbis -logg
VORBISDLWIN=vorbisfile.dll
VORBISDLLIN=libvorbisfile.so

# FLAC
# http://flac.sf.net/
USEFLAC=0
LINKFLAC=0
FLACCFLAGS=
FLACLIBS=-lflac
FLACDLWIN=libflac.dll
FLACDLLIN=libFLAC.so

# MPEG Audio via mpadec
# http://mpadec.sf.net/
USEMPADEC=1
LINKMPADEC=1
MPADECCFLAGS=-Impadec
MPADECLIBS=mpadec/libmpadec.a
MPADECDLWIN=mpadec.dll
MPADECDLLIN=libmpadec.so

# libSDL 1.2 or better (used just for the test app on Win32, for CDA and sound everywhere else)
# http://www.libsdl.org/
SDLCFLAGS=$(shell sdl-config --cflags)
SDLLIBS=$(shell sdl-config --libs)

# Directories for library SDKs on Windows
# I keep my third-party libraries each in a directory on their own.
SDKDIRECTX=c:/sdks/directx/dx61/
SDKOPENAL=c:/sdks/OpenAL/
SDKVORBIS=c:/sdks/oggvorbis-gcc3/
SDKFLAC=c:/sdks/flac/

obj=obj
src=src
inc=inc
o=o
libfile=libjfaud.a


# Linux-specific options
ifeq ($(shell uname -s),Linux)
exe=
platformcflags=$(SDLCFLAGS)
platformobjs=$(obj)/cda_sdl.$o $(obj)/waveout_sdl.$o
platformlibs=
SDLCFLAGS+= -DSDL_main=main
ALCFLAGS:= $(ALCFLAGS) -DALDL=\"$(ALDLLIN)\"
VORBISCFLAGS:= $(VORBISCFLAGS) -DVORBISDL=\"$(VORBISDLLIN)\"
FLACCFLAGS:= $(FLACCFLAGS) -DFLACDL=\"$(FLACDLLIN)\"
MPADECCFLAGS:=$(MPADECCFLAGS) -DMPADECDL=\"$(MPADECDLLIN)\"
MPADECBUILD=libmpadec.so
endif

# *BSD-specific options
ifeq ($(findstring BSD,$(shell uname -s)),BSD)
exe=
platformcflags=$(SDLCFLAGS)
platformobjs=$(obj)/cda_sdl.$o $(obj)/waveout_sdl.$o
platformlibs=
ALCFLAGS:= $(ALCFLAGS) -DALDL=\"$(ALDLLIN)\"
VORBISCFLAGS:= $(VORBISCFLAGS) -DVORBISDL=\"$(VORBISDLLIN)\"
FLACCFLAGS:= $(FLACCFLAGS) -DFLACDL=\"$(FLACDLLIN)\"
MPADECCFLAGS:=$(MPADECCFLAGS) -DMPADECDL=\"$(MPADECDLLIN)\"
MPADECBUILD=libmpadec.so
endif

# MinGW32-specific options
ifeq ($(findstring MINGW,$(shell uname -s)),MINGW)
exe=.exe
platformcflags=-I$(SDKDIRECTX)include $(DIRECTXCFLAGS)
platformobjs=$(obj)/cda_win32.$o $(obj)/midisynth_win32.$o $(obj)/waveout_dsound.$o
platformlibs=-lwinmm -L$(SDKDIRECTX)lib $(DIRECTXLIBS)

ALCFLAGS:=-I$(SDKOPENAL)Include $(ALCFLAGS) -DALDL=\"$(ALDLWIN)\"
ALLIBS=-L$(SDKOPENAL)libs -lopenal32

VORBISCFLAGS:=-I$(SDKVORBIS)include $(VORBISCFLAGS) -DVORBISDL=\"$(VORBISDLWIN)\"
VORBISLIBS:=-L$(SDKVORBIS)lib $(VORBISLIBS)

FLACCFLAGS:=-I$(SDKFLAC)include $(FLACCFLAGS) -DFLACDL=\"$(FLACDLWIN)\"
FLACLIBS:=-L$(SDKFLAC)bin $(FLACLIBS)

MPADECCFLAGS:=$(MPADECCFLAGS) -DMPADECDL=\"$(MPADECDLWIN)\"
MPADECBUILD=mpadec.dll
endif

OURCFLAGS=-fno-rtti -fno-exceptions -g -I$(inc) $(platformcflags) #-DDEBUG
LIBS=$(platformlibs)
dependencies=

OURCFLAGS+= -DUSEAL=$(USEAL)
ifeq ($(USEAL),1)
OURCFLAGS+= -DLINKAL=$(LINKAL)
ifeq ($(LINKAL),1)
LIBS+= $(ALLIBS)
endif
endif

OURCFLAGS+= -DUSEVORBIS=$(USEVORBIS)
ifeq ($(USEVORBIS),1)
OURCFLAGS+= -DLINKVORBIS=$(LINKVORBIS)
ifeq ($(LINKVORBIS),1)
LIBS+= $(VORBISLIBS)
endif
endif

OURCFLAGS+= -DUSEFLAC=$(USEFLAC)
ifeq ($(USEFLAC),1)
OURCFLAGS+= -DLINKFLAC=$(LINKFLAC)
ifeq ($(LINKFLAC),1)
LIBS+= $(FLACLIBS)
endif
endif

OURCFLAGS+= -DUSEMPADEC=$(USEMPADEC)
ifeq ($(USEMPADEC),1)
OURCFLAGS+= -DLINKMPADEC=$(LINKMPADEC)
ifeq ($(LINKMPADEC),1)
LIBS+= $(MPADECLIBS)
endif
dependencies+= libmpadec
endif

# JFAud can be built natively in both a 64bit and 32bit environment
# but the JF ports that use it are currently only 32bit games, so
# setting NBIT=32 on the make command line will force a 32bit compile
NBIT=32
ifeq ($(NBIT),32)
ifeq ($(findstring x86_64,$(shell uname -m)),x86_64)
	# on my 64bit Gentoo these are the 32bit emulation libs
	LIBS+= -m32 -L/emul/linux/x86/usr/lib
	OURCFLAGS+= -m32
endif
endif

CC=gcc
CXX=gcc
AR=ar
OURCXXFLAGS=$(OURCFLAGS)

libraryobjs=$(obj)/jfaud.$o $(obj)/soundcache.$o \
	    $(obj)/mixer.$o $(obj)/nullmixer.$o $(obj)/almixer.$o $(obj)/softwaremixer.$o \
	    $(obj)/file.$o $(obj)/stdfile.$o \
	    $(obj)/pcmbuffer.$o $(obj)/midibuffer.$o \
	    $(obj)/soundfile.$o $(obj)/waveformfile.$o $(obj)/midifile.$o \
            $(obj)/waveformfile_raw.$o $(obj)/waveformfile_riffwave.$o $(obj)/waveformfile_aiff.$o \
            $(obj)/waveformfile_voc.$o $(obj)/waveformfile_au.$o $(obj)/waveformfile_oggvorbis.$o \
            $(obj)/waveformfile_flac.$o $(obj)/waveformfile_mpeg.$o \
	    $(obj)/midifile_smf.$o $(obj)/midiseq.$o $(obj)/midisynth.$o \
            $(obj)/cda_null.$o $(obj)/waveout.$o $(obj)/dynlib.$o $(obj)/crc32.$o $(platformobjs)
testprogobjs=$(obj)/jfaudtest.$o $(libfile)

# module-specific CFLAGS declaration variables
almixer_cpp_CFLAGS=$(ALCFLAGS)
jfaud_cpp_CFLAGS=$(ALCFLAGS)
waveformfile_oggvorbis_cpp_CFLAGS=$(VORBISCFLAGS)
waveformfile_flac_cpp_CFLAGS=$(FLACCFLAGS)
waveformfile_mpeg_cpp_CFLAGS=$(MPADECCFLAGS)
waveformfile_cpp_CFLAGS=$(VORBISCFLAGS) $(FLACCFLAGS) $(MPADECCFLAGS)


.PHONY: all test clean veryclean
all: $(libfile)
test: jfaudtest$(exe)

jfaudtest$(exe): $(testprogobjs) $(dependencies)
	$(CXX) -g -o $@ $(testprogobjs) $(SDLLIBS) $(LIBS) -lm -lsupc++

$(libfile): $(libraryobjs)
	$(AR) cr $@ $^
	ranlib $@

sinclude Makefile.deps

$(obj)/jfaudtest.$o: $(src)/jfaudtest.cpp $(inc)/jfaud.hpp $(inc)/file.hpp $(inc)/mixer.hpp $(inc)/waveformfile.hpp $(inc)/soundfile.hpp $(inc)/pcmbuffer.hpp $(inc)/buffer.hpp $(inc)/cda.hpp $(inc)/sysdefs.h
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $(SDLCFLAGS) -c -o $@ $<

$(obj)/%.$o: $(src)/%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $($(subst .,_,$(notdir $<))_CFLAGS) -c -o $@ $<

# for reconstructing the dependencies makefile
.PHONY: deps
deps: $(libraryobjs:.$o=.dep)
	perl processdeps.pl $(obj) $(src) $(inc) $^ > Makefile.deps.new
	rm -f $^
$(obj)/%.dep: $(src)/%.cpp
	$(CXX) $(CXXFLAGS) $(OURCXXFLAGS) $($(subst .,_,$(notdir $<))_CFLAGS) -MM -c -o $@ $<

# housekeeping
clean:
	-rm -f $(libraryobjs) jfaudtest$(exe) $(obj)/jfaudtest.$o
	-cd mpadec && $(MAKE) clean
veryclean: clean
	-rm -f $(libfile)

# for building libmpadec
.PHONY: libmpadec
libmpadec:
	cd mpadec && $(MAKE) libmpadec.a $(MPADECBUILD) NBIT=$(NBIT)
