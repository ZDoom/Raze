# EDuke32 Makefile for Microsoft NMake

root=..\..\

obj=$(root)\obj
source=$(root)\source

ENGINE=build
ENGINE_ROOT=$(source)\$(ENGINE)
ENGINE_SRC=$(ENGINE_ROOT)\src
ENGINE_INC=$(ENGINE_ROOT)\include
ENGINE_OBJ=$(obj)\$(ENGINE)

DUKE3D=duke3d
DUKE3D_ROOT=$(source)\$(DUKE3D)
DUKE3D_SRC=$(DUKE3D_ROOT)\src
DUKE3D_OBJ=$(obj)\$(DUKE3D)
DUKE3D_RSRC=$(DUKE3D_ROOT)\rsrc

MACT=mact
MACT_ROOT=$(source)\$(MACT)
MACT_SRC=$(MACT_ROOT)\src
MACT_INC=$(MACT_ROOT)\include
MACT_OBJ=$(obj)\$(MACT)

AUDIOLIB=audiolib
AUDIOLIB_ROOT=$(source)\$(AUDIOLIB)
AUDIOLIB_OBJ=$(obj)\$(AUDIOLIB)
AUDIOLIB_INC=$(AUDIOLIB_ROOT)\include
AUDIOLIB_SRC=$(AUDIOLIB_ROOT)\src

ENET=enet
ENET_ROOT=$(source)\$(ENET)
ENET_OBJ=$(obj)\$(ENET)
ENET_INC=$(ENET_ROOT)\include
ENET_SRC=$(ENET_ROOT)\src

GLAD=glad
GLAD_ROOT=$(source)\$(GLAD)
GLAD_OBJ=$(obj)\$(GLAD)
GLAD_INC=$(GLAD_ROOT)\include
GLAD_SRC=$(GLAD_ROOT)\src

LIBXMPLITE=libxmp-lite
LIBXMPLITE_ROOT=$(source)\$(LIBXMPLITE)
LIBXMPLITE_OBJ=$(obj)\$(LIBXMPLITE)
LIBXMPLITE_INC=$(LIBXMPLITE_ROOT)\include
LIBXMPLITE_SRC=$(LIBXMPLITE_ROOT)\src

o=obj
res=res
asm=masm




CPLUSPLUS=1

!ifndef WINBITS
WINBITS=32
!endif

WINLIB=\$(WINBITS)

!if ($(WINBITS)==32)
WINMACHINE=/MACHINE:X86
!elseif ($(WINBITS)==64)
WINMACHINE=/MACHINE:X64
!endif

# the WDK allows us to link against msvcrt.dll instead of msvcrxxx.dll
# WDKROOT="H:\WinDDK\7600.16385.1"
PLATFORM=$(root)\platform\Windows

!ifndef RENDERTYPE
RENDERTYPE=WIN
!endif
!ifndef MIXERTYPE
MIXERTYPE=WIN
!endif

!ifdef DEBUG
# debugging options
flags_cl=/Od /Zi # /analyze
flags_link=/DEBUG
!else
# release options
flags_cl=/O2 /GL /MP # /I$(WDKROOT)\inc\crt /I$(WDKROOT)\inc\api
!if ($(WINBITS)!=64)
flags_cl=$(flags_cl) /arch:SSE
!endif
flags_link=/RELEASE /LTCG # /LIBPATH:$(WDKROOT)\lib\wxp\i386 /LIBPATH:$(WDKROOT)\lib\Crt\i386
!endif

ENGINEOPTS=/DUSE_OPENGL /DPOLYMER /DSTARTUP_WINDOW /DUSE_LIBVPX /DHAVE_VORBIS /DHAVE_XMP

!ifdef CPLUSPLUS
ENGINEOPTS=$(ENGINEOPTS) /TP
!endif

!if ($(WINBITS)==64)
NOASM=1
!endif

CC=cl
AS=ml
LINK=link /nologo /opt:ref
MT=mt
CFLAGS= /MT /J /nologo /std:c++latest $(flags_cl)  \
	/I$(DUKE3D_SRC) /I$(ENGINE_INC)\msvc /I$(ENGINE_INC) /I$(MACT_INC) /I$(AUDIOLIB_INC) /I$(ENET_INC) /I$(GLAD_INC) /I$(LIBXMPLITE_INC) \
	/W2 $(ENGINEOPTS) \
	/I$(PLATFORM)\include /DRENDERTYPE$(RENDERTYPE)=1 /DMIXERTYPE$(MIXERTYPE)=1 /DSDL_USEFOLDER /DSDL_TARGET=2

ENET_CFLAGS=/I$(ENET_INC) /I$(ENET_SRC)
LIBXMPLITE_CFLAGS=/I$(LIBXMPLITE_INC) /I$(LIBXMPLITE_INC)/libxmp-lite /I$(LIBXMPLITE_SRC) -DHAVE_ROUND -DLIBXMP_CORE_PLAYER -DBUILDING_STATIC
AUDIOLIB_CFLAGS=/I$(AUDIOLIB_INC) /I$(AUDIOLIB_SRC)

LIBS=user32.lib gdi32.lib shell32.lib winmm.lib ws2_32.lib comctl32.lib shlwapi.lib oleaut32.lib ole32.lib imm32.lib SetupAPI.Lib version.lib \
     libogg.a libvorbis.a libvorbisfile.a libxmp-lite.a libvpx.a dxguid.lib dsound.lib advapi32.lib libcompat-to-msvc.a

!if ("$(RENDERTYPE)"=="SDL")
LIBS=libSDL2main.a libSDL2.a $(LIBS)
!endif

LIBS=/NODEFAULTLIB:glu32.lib /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib /NODEFAULTLIB:libcmt.lib \
    /NODEFAULTLIB:libcmtd.lib $(LIBS)

# NOASM     When defined, uses C instead of assembly code
!ifdef NOASM
CFLAGS=$(CFLAGS) /DNOASM
!endif

ASFLAGS=/nologo /coff /c
EXESUFFIX=.exe
!ifdef DEBUG
CFLAGS=$(CFLAGS) /DDEBUGGINGAIDS /D "_CRT_SECURE_NO_DEPRECATE"
LIBS=$(LIBS)  msvcrtd.lib
!else
# comment msvcrt_winxp.obj if not using the WDK
LIBS=$(LIBS)  msvcrt.lib # msvcrt_winxp.obj
!endif

ENGINE_OBJS= \
!ifdef NOASM
        $(ENGINE_OBJ)\a-c.$o \
!else
        $(ENGINE_OBJ)\a.$o \
!endif
	$(ENGINE_OBJ)\animvpx.$o \
	$(ENGINE_OBJ)\baselayer.$o \
	$(ENGINE_OBJ)\cache1d.$o \
	$(ENGINE_OBJ)\klzw.$o \
	$(ENGINE_OBJ)\common.$o \
	$(ENGINE_OBJ)\compat.$o \
	$(ENGINE_OBJ)\crc32.$o \
	$(ENGINE_OBJ)\defs.$o \
	$(ENGINE_OBJ)\colmatch.$o \
	$(ENGINE_OBJ)\engine.$o \
	$(ENGINE_OBJ)\clip.$o \
	$(ENGINE_OBJ)\2d.$o \
        $(ENGINE_OBJ)\tiles.$o \
	$(ENGINE_OBJ)\hash.$o \
	$(ENGINE_OBJ)\palette.$o \
        $(ENGINE_OBJ)\glbuild.$o \
	$(ENGINE_OBJ)\glsurface.$o \
        $(ENGINE_OBJ)\texcache.$o \
        $(ENGINE_OBJ)\kplib.$o \
        $(ENGINE_OBJ)\hightile.$o \
	$(ENGINE_OBJ)\polymost.$o \
        $(ENGINE_OBJ)\polymer.$o \
        $(ENGINE_OBJ)\mdsprite.$o \
        $(ENGINE_OBJ)\voxmodel.$o \
	$(ENGINE_OBJ)\tilepacker.$o \
	$(ENGINE_OBJ)\dxtfilter.$o \
	$(ENGINE_OBJ)\textfont.$o \
	$(ENGINE_OBJ)\smalltextfont.$o \
	$(ENGINE_OBJ)\lz4.$o \
	$(ENGINE_OBJ)\md4.$o \
	$(ENGINE_OBJ)\mmulti_null.$o \
	$(ENGINE_OBJ)\osd.$o \
	$(ENGINE_OBJ)\pragmas.$o \
	$(ENGINE_OBJ)\rev.$o \
	$(ENGINE_OBJ)\scriptfile.$o \
	$(ENGINE_OBJ)\mutex.$o \
	$(ENGINE_OBJ)\winbits.$o \
	$(ENGINE_OBJ)\xxhash.$o \
	$(ENGINE_OBJ)\screenshot.$o \
	$(ENGINE_OBJ)\softsurface.$o \
	$(ENGINE_OBJ)\mhk.$o \
	$(ENGINE_OBJ)\pngwrite.$o \
	$(ENGINE_OBJ)\miniz.$o \
	$(ENGINE_OBJ)\miniz_tinfl.$o \
	$(ENGINE_OBJ)\miniz_tdef.$o \
	$(ENGINE_OBJ)\fix16.$o \
	$(ENGINE_OBJ)\fix16_str.$o


ENGINE_EDITOR_OBJS=$(ENGINE_OBJ)\build.$o \
	$(ENGINE_OBJ)\startwin.editor.$o \
	$(ENGINE_OBJ)\config.$o

ENET_OBJS=$(ENET_OBJ)\callbacks.$o \
	$(ENET_OBJ)\host.$o \
	$(ENET_OBJ)\list.$o \
	$(ENET_OBJ)\packet.$o \
	$(ENET_OBJ)\peer.$o \
	$(ENET_OBJ)\protocol.$o \
	$(ENET_OBJ)\win32.$o \
	$(ENET_OBJ)\compress.$o

GLAD_OBJS=$(GLAD_OBJ)\glad.$o \
!if ("$(RENDERTYPE)"=="WIN")
          $(GLAD_OBJ)\glad_wgl.$o 
!endif

LIBXMPLITE_OBJS=$(LIBXMPLITE_OBJ)\control.$o \
    $(LIBXMPLITE_OBJ)\dataio.$o \
    $(LIBXMPLITE_OBJ)\effects.$o \
    $(LIBXMPLITE_OBJ)\filter.$o \
    $(LIBXMPLITE_OBJ)\format.$o \
    $(LIBXMPLITE_OBJ)\hio.$o \
    $(LIBXMPLITE_OBJ)\lfo.$o \
    $(LIBXMPLITE_OBJ)\load.$o \
    $(LIBXMPLITE_OBJ)\load_helpers.$o \
    $(LIBXMPLITE_OBJ)\memio.$o \
    $(LIBXMPLITE_OBJ)\mixer.$o \
    $(LIBXMPLITE_OBJ)\mix_all.$o \
    $(LIBXMPLITE_OBJ)\period.$o \
    $(LIBXMPLITE_OBJ)\player.$o \
    $(LIBXMPLITE_OBJ)\read_event.$o \
    $(LIBXMPLITE_OBJ)\scan.$o \
    $(LIBXMPLITE_OBJ)\smix.$o \
    $(LIBXMPLITE_OBJ)\virtual.$o \
    $(LIBXMPLITE_OBJ)\common.$o \
    $(LIBXMPLITE_OBJ)\itsex.$o \
    $(LIBXMPLITE_OBJ)\it_load.$o \
    $(LIBXMPLITE_OBJ)\mod_load.$o \
    $(LIBXMPLITE_OBJ)\mtm_load.$o \
    $(LIBXMPLITE_OBJ)\s3m_load.$o \
    $(LIBXMPLITE_OBJ)\sample.$o \
    $(LIBXMPLITE_OBJ)\xm_load.$o \

AUDIOLIB_OBJS=$(AUDIOLIB_OBJ)\drivers.$o \
	$(AUDIOLIB_OBJ)\fx_man.$o \
	$(AUDIOLIB_OBJ)\multivoc.$o \
	$(AUDIOLIB_OBJ)\mix.$o \
	$(AUDIOLIB_OBJ)\mixst.$o \
	$(AUDIOLIB_OBJ)\pitch.$o \
	$(AUDIOLIB_OBJ)\formats.$o \
	$(AUDIOLIB_OBJ)\vorbis.$o \
	$(AUDIOLIB_OBJ)\flac.$o \
	$(AUDIOLIB_OBJ)\xa.$o \
	$(AUDIOLIB_OBJ)\xmp.$o \
	$(AUDIOLIB_OBJ)\driver_nosound.$o

MACT_OBJS= \
	$(MACT_OBJ)\control.$o \
	$(MACT_OBJ)\keyboard.$o \
	$(MACT_OBJ)\joystick.$o \
	$(MACT_OBJ)\scriplib.$o \
	$(MACT_OBJ)\animlib.$o

DUKE3D_OBJS=$(DUKE3D_OBJ)\game.$o \
	$(DUKE3D_OBJ)\actors.$o \
	$(DUKE3D_OBJ)\anim.$o \
        $(DUKE3D_OBJ)\cheats.$o \
        $(DUKE3D_OBJ)\sbar.$o \
        $(DUKE3D_OBJ)\screentext.$o \
        $(DUKE3D_OBJ)\screens.$o \
        $(DUKE3D_OBJ)\cmdline.$o \
	$(DUKE3D_OBJ)\common.$o \
	$(DUKE3D_OBJ)\demo.$o \
	$(DUKE3D_OBJ)\gamedef.$o \
	$(DUKE3D_OBJ)\gameexec.$o \
	$(DUKE3D_OBJ)\gamevars.$o \
	$(DUKE3D_OBJ)\global.$o \
	$(DUKE3D_OBJ)\input.$o \
	$(DUKE3D_OBJ)\menus.$o \
	$(DUKE3D_OBJ)\namesdyn.$o \
    $(DUKE3D_OBJ)\network.$o \
	$(DUKE3D_OBJ)\player.$o \
	$(DUKE3D_OBJ)\premap.$o \
	$(DUKE3D_OBJ)\savegame.$o \
	$(DUKE3D_OBJ)\sector.$o \
	$(DUKE3D_OBJ)\rts.$o \
	$(DUKE3D_OBJ)\config.$o \
	$(DUKE3D_OBJ)\osdfuncs.$o \
	$(DUKE3D_OBJ)\osdcmds.$o \
	$(DUKE3D_OBJ)\grpscan.$o \
	$(DUKE3D_OBJ)\winbits.$o \
	$(DUKE3D_OBJ)\gameres.$(res) \
	$(DUKE3D_OBJ)\startwin.game.$o \
	$(DUKE3D_OBJ)\sounds.$o \
	$(DUKE3D_OBJ)\soundsdyn.$o \
!ifdef DEBUG
	$(DUKE3D_OBJ)\mdump.$o
!endif

DUKE3D_EDITOR_OBJS=$(DUKE3D_OBJ)\astub.$o \
	$(DUKE3D_OBJ)\common.$o \
	$(DUKE3D_OBJ)\grpscan.$o \
	$(DUKE3D_OBJ)\m32common.$o \
	$(DUKE3D_OBJ)\m32def.$o \
	$(DUKE3D_OBJ)\m32vars.$o \
	$(DUKE3D_OBJ)\m32exec.$o \
	$(DUKE3D_OBJ)\sounds_mapster32.$o \
	$(DUKE3D_OBJ)\buildres.$(res) \
!ifdef DEBUG
	$(DUKE3D_OBJ)\mdump.$o
!endif

!if ("$(RENDERTYPE)"=="WIN")
ENGINE_OBJS=$(ENGINE_OBJS) $(ENGINE_OBJ)\winlayer.$o $(ENGINE_OBJ)\rawinput.$o
!endif
!if ("$(RENDERTYPE)"=="SDL")
ENGINE_OBJS=$(ENGINE_OBJS) $(ENGINE_OBJ)\sdlayer.$o
DUKE3D_OBJS=$(DUKE3D_OBJS) $(DUKE3D_OBJ)\game_icon.$o
DUKE3D_EDITOR_OBJS=$(DUKE3D_EDITOR_OBJS) $(DUKE3D_OBJ)\build_icon.$o
!endif

!if ("$(MIXERTYPE)"=="WIN")
DUKE3D_OBJS=$(DUKE3D_OBJS) $(DUKE3D_OBJ)\midi.$o $(DUKE3D_OBJ)\music.$o $(DUKE3D_OBJ)\mpu401.$o
AUDIOLIB_OBJS=$(AUDIOLIB_OBJS) $(AUDIOLIB_OBJ)\driver_directsound.$o
!endif
!if ("$(MIXERTYPE)"=="SDL")
DUKE3D_OBJS=$(DUKE3D_OBJS) $(DUKE3D_OBJ)\sdlmusic.$o
AUDIOLIB_OBJS=$(AUDIOLIB_OBJS) $(AUDIOLIB_OBJ)/driver_sdl.$o
!endif

DUKE3D_OBJS=$(DUKE3D_OBJS) $(MUSICOBJ)
DUKE3D_EDITOR_OBJS=$(DUKE3D_EDITOR_OBJS) $(MUSICOBJ)


CHECKDIR_ENGINE=@if not exist "$(ENGINE_OBJ)" mkdir "$(ENGINE_OBJ)"
CHECKDIR_DUKE3D=@if not exist "$(DUKE3D_OBJ)" mkdir "$(DUKE3D_OBJ)"
CHECKDIR_ENET=@if not exist "$(ENET_OBJ)" mkdir "$(ENET_OBJ)"
CHECKDIR_GLAD=@if not exist "$(GLAD_OBJ)" mkdir "$(GLAD_OBJ)"
CHECKDIR_MACT=@if not exist "$(MACT_OBJ)" mkdir "$(MACT_OBJ)"
CHECKDIR_AUDIOLIB=@if not exist "$(AUDIOLIB_OBJ)" mkdir "$(AUDIOLIB_OBJ)"
CHECKDIR_LIBXMPLITE=@if not exist "$(LIBXMPLITE_OBJ)" mkdir "$(LIBXMPLITE_OBJ)"

EDUKE32_TARGET=$(root)\eduke32$(EXESUFFIX)
MAPSTER32_TARGET=$(root)\mapster32$(EXESUFFIX)

# RULES
.SUFFIXES: .masm

{$(ENGINE_SRC)}.masm{$(ENGINE_OBJ)}.$o:
	$(CHECKDIR_ENGINE)
	$(AS) /c $(ASFLAGS) /Fo$@ $<

{$(ENGINE_SRC)}.cpp{$(ENGINE_OBJ)}.$o:
	$(CHECKDIR_ENGINE)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(ENGINE_SRC)}.c{$(ENGINE_OBJ)}.$o:
	$(CHECKDIR_ENGINE)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(ENET_SRC)}.c{$(ENET_OBJ)}.$o:
	$(CHECKDIR_ENET)
	$(CC) /c $(CFLAGS) $(ENET_CFLAGS) /Fo$@ $<

{$(GLAD_SRC)}.c{$(GLAD_OBJ)}.$o:
	$(CHECKDIR_GLAD)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(LIBXMPLITE_SRC)}.c{$(LIBXMPLITE_OBJ)}.$o:
	$(CHECKDIR_LIBXMPLITE)
	$(CC) /c $(CFLAGS) $(LIBXMPLITE_CFLAGS) /Fo$@ $<

{$(AUDIOLIB_SRC)}.cpp{$(AUDIOLIB_OBJ)}.$o:
	$(CHECKDIR_AUDIOLIB)
	$(CC) /c $(CFLAGS) $(AUDIOLIB_CFLAGS) /Fo$@ $<

{$(MACT_SRC)\}.cpp{$(MACT_OBJ)\}.$o:
	$(CHECKDIR_MACT)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(DUKE3D_RSRC)\}.c{$(DUKE3D_OBJ)\}.$o:
	$(CHECKDIR_DUKE3D)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(DUKE3D_SRC)\}.cpp{$(DUKE3D_OBJ)\}.$o:
	$(CHECKDIR_DUKE3D)
	$(CC) /c $(CFLAGS) /Fo$@ $<

{$(DUKE3D_RSRC)\}.rc{$(DUKE3D_OBJ)\}.$(res):
	$(CHECKDIR_DUKE3D)
	$(RC) /i$(ENGINE_INC)\ /i$(DUKE3D_SRC)\ /i$(DUKE3D_RSRC)\ /DPOLYMER /fo$@ /r $<


# TARGETS


all: $(EDUKE32_TARGET) $(MAPSTER32_TARGET)

$(EDUKE32_TARGET): $(DUKE3D_OBJS) $(ENGINE_OBJS) $(AUDIOLIB_OBJS) $(MACT_OBJS) $(ENET_OBJS) $(GLAD_OBJS) $(LIBXMPLITE_OBJS)
	$(LINK) /OUT:$@ /SUBSYSTEM:WINDOWS $(WINMACHINE) /LIBPATH:$(PLATFORM)\lib$(WINLIB) $(flags_link) /MAP $** $(LIBS)
	$(MT) -manifest $(DUKE3D_RSRC)\manifest.game.xml -hashupdate -outputresource:$@ -out:$@.manifest

$(MAPSTER32_TARGET): $(DUKE3D_EDITOR_OBJS) $(ENGINE_OBJS) $(ENGINE_EDITOR_OBJS) $(AUDIOLIB_OBJS) $(GLAD_OBJS)
	$(LINK) /OUT:$@ /SUBSYSTEM:WINDOWS $(WINMACHINE) /LIBPATH:$(PLATFORM)\lib$(WINLIB) $(flags_link) /MAP $** $(LIBS)
	$(MT) -manifest $(DUKE3D_RSRC)\manifest.build.xml -hashupdate -outputresource:$@ -out:$@.manifest

!include $(DUKE3D_ROOT)\Dependencies.mak
!include $(ENGINE_ROOT)\Dependencies.mak

# PHONIES

clean:
	-del /Q $(EDUKE32_TARGET) $(MAPSTER32_TARGET) $(DUKE3D_OBJS) $(DUKE3D_EDITOR_OBJS) $(ENGINE_OBJS) $(ENGINE_EDITOR_OBJS) *.pdb $(root)\*.pdb $(root)\*.map $(root)\*.manifest
    -del /Q $(ENET_OBJS) $(LIBXMPLITE_OBJS) $(MACT_OBJS) $(AUDIOLIB_OBJS) $(GLAD_OBJS)
veryclean: clean
