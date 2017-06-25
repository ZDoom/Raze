LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(findstring clang,$(NDK_TOOLCHAIN_VERSION)),)
    FOUND_CLANG := 0
else
    FOUND_CLANG := 1
endif

LOCAL_MODULE    := duke3d

COMMONFLAGS     := -x c++ -std=gnu++11 -fvisibility=hidden -fPIC -funsigned-char -fno-strict-aliasing -pthread \
                   -W -Wall -Wextra -Wpointer-arith -Wno-char-subscripts -Wno-missing-braces -Wwrite-strings -Wuninitialized \
                   -Wno-attributes -Wno-strict-overflow -Wno-unused-result -Wlogical-op -Wcast-qual -Werror=return-type \
                   -DHAVE_VORBIS -DHAVE_JWZGLES -DHAVE_ANDROID -DRENDERTYPESDL=1 -DMIXERTYPESDL=1 -DUSE_OPENGL -DNETCODE_DISABLE -DUSE_LIBVPX \
                   -D_GNU_SOURCE=1 -D_REENTRANT

ifeq ($(FOUND_CLANG),1)
    COMMONFLAGS += -Wno-unknown-warning-option -Wno-deprecated-register
endif

LOCAL_LDFLAGS   := -fuse-ld=bfd
LOCAL_ARM_NEON  = true

ifeq ($(NDK_DEBUG), 1)
    COMMONFLAGS += -O0 -ggdb -fno-omit-frame-pointer -fno-stack-protector -D_FORTIFY_SOURCE=0 -DDEBUGGINGAIDS=0
else
    COMMONFLAGS += -O2 -DNDEBUG -D_FORTIFY_SOURCE=2
    ifeq ($(FOUND_CLANG),0)
        COMMONFLAGS += -DUSING_LTO -flto
        LOCAL_LDFLAGS += -flto
    endif
endif

LOCAL_CFLAGS    = $(COMMONFLAGS)
LOCAL_CPPFLAGS  = $(COMMONFLAGS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/source/duke3d/src $(LOCAL_PATH)/source/mact/include $(LOCAL_PATH)/source/audiolib/include $(LOCAL_PATH)/source/enet/include $(LOCAL_PATH)/source/build/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/source/etcpak/include
LOCAL_C_INCLUDES += $(TOP_DIR)/ $(TOP_DIR)/Libraries/liboggvorbis/include $(TOP_DIR)/Libraries/ $(TOP_DIR)/Libraries/SDL2/include $(TOP_DIR)/Libraries/SDL2_mixer/include $(TOP_DIR)/Libraries/TinyXML/include $(TOP_DIR)/TouchControls $(TOP_DIR)/Libraries/libvpx/include

ANDROID_SRC = \
    source/build/src/jwzgles.c \
    platform/Android/Duke3d/jni/android-jni.cpp \
    source/etcpak/src/ProcessRGB.cpp \
    source/etcpak/src/Tables.cpp \
    source/duke3d/src/in_android.cpp \

BUILD_SRC = \
    source/build/src/2d.cpp \
    source/build/src/a-c.cpp \
    source/build/src/animvpx.cpp \
    source/build/src/baselayer.cpp \
    source/build/src/cache1d.cpp \
    source/build/src/compat.cpp \
    source/build/src/common.cpp \
    source/build/src/crc32.cpp \
    source/build/src/defs.cpp \
    source/build/src/clip.cpp \
    source/build/src/colmatch.cpp \
    source/build/src/engine.cpp \
    source/build/src/hash.cpp \
    source/build/src/glbuild.cpp \
    source/build/src/polymost.cpp \
    source/build/src/mdsprite.cpp \
    source/build/src/texcache.cpp \
    source/build/src/dxtfilter.cpp \
    source/build/src/hightile.cpp \
    source/build/src/textfont.cpp \
    source/build/src/smalltextfont.cpp \
    source/build/src/kplib.cpp \
    source/build/src/mmulti_null.cpp \
    source/build/src/lz4.c \
    source/build/src/osd.cpp \
    source/build/src/md4.cpp \
    source/build/src/pragmas.cpp \
    source/build/src/scriptfile.cpp \
    source/build/src/mutex.cpp \
    source/build/src/xxhash.c \
    source/build/src/voxmodel.cpp \
    source/build/src/rev.cpp \
    source/build/src/sdlayer.cpp \
    source/build/src/screenshot.cpp \
    source/build/src/tiles.cpp \
    source/build/src/mhk.cpp \
    source/build/src/palette.cpp \

MACT_SRC = \
    source/mact/src/file_lib.cpp \
    source/mact/src/control.cpp \
    source/mact/src/keyboard.cpp \
    source/mact/src/joystick.cpp \
    source/mact/src/scriplib.cpp \
    source/mact/src/animlib.cpp \

GAME_SRC = \
    source/duke3d/src/game.cpp \
    source/duke3d/src/actors.cpp \
    source/duke3d/src/anim.cpp \
    source/duke3d/src/common.cpp \
    source/duke3d/src/config.cpp \
    source/duke3d/src/demo.cpp \
    source/duke3d/src/gamedef.cpp \
    source/duke3d/src/gameexec.cpp \
    source/duke3d/src/gamevars.cpp \
    source/duke3d/src/global.cpp \
    source/duke3d/src/input.cpp \
    source/duke3d/src/menus.cpp \
    source/duke3d/src/namesdyn.cpp \
    source/duke3d/src/net.cpp \
    source/duke3d/src/player.cpp \
    source/duke3d/src/premap.cpp \
    source/duke3d/src/savegame.cpp \
    source/duke3d/src/sector.cpp \
    source/duke3d/src/rts.cpp \
    source/duke3d/src/osdfuncs.cpp \
    source/duke3d/src/osdcmds.cpp \
    source/duke3d/src/grpscan.cpp \
    source/duke3d/src/sounds.cpp \
    source/duke3d/src/soundsdyn.cpp \
    source/duke3d/src/sdlmusic.cpp \
    source/duke3d/src/cmdline.cpp \
    source/duke3d/src/screens.cpp \
    source/duke3d/src/screentext.cpp \
    source/duke3d/src/cheats.cpp \
    source/duke3d/src/sbar.cpp \

AUDIOLIB_SRC = \
    source/audiolib/src/drivers.cpp \
    source/audiolib/src/fx_man.cpp \
    source/audiolib/src/multivoc.cpp \
    source/audiolib/src/mix.cpp \
    source/audiolib/src/mixst.cpp \
    source/audiolib/src/pitch.cpp \
    source/audiolib/src/formats.cpp \
    source/audiolib/src/vorbis.cpp \
    source/audiolib/src/flac.cpp \
    source/audiolib/src/xa.cpp \
    source/audiolib/src/xmp.cpp \
    source/audiolib/src/driver_nosound.cpp \
    source/audiolib/src/driver_sdl.cpp \

ENET_SRC = \
    source/enet/src/callbacks.c \
    source/enet/src/host.c \
    source/enet/src/list.c \
    source/enet/src/packet.c \
    source/enet/src/peer.c \
    source/enet/src/protocol.c \
    source/enet/src/compress.c \
    source/enet/src/unix.c \

LOCAL_SRC_FILES         = $(ANDROID_SRC) $(AUDIOLIB_SRC) $(MACT_SRC) $(GAME_SRC) $(BUILD_SRC)

LOCAL_LDLIBS            := -lGLESv1_CM -lEGL -ldl -llog
LOCAL_STATIC_LIBRARIES  := touch
LOCAL_SHARED_LIBRARIES  := ogg vorbis SDL2 SDL2_mixer libvpx

include $(BUILD_SHARED_LIBRARY)
