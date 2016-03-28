LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := duke3d

COMMONFLAGS     := -x c++ -std=gnu++11 -fvisibility=hidden -fPIC -funsigned-char -fno-strict-aliasing -pthread -mhard-float \
                   -W -Wall -Wextra -Wpointer-arith -Wno-char-subscripts -Wno-missing-braces -Wwrite-strings -Wuninitialized \
                   -Wno-attributes -Wno-strict-overflow -Wno-unused-result -Wlogical-op -Wcast-qual \
                   -DHAVE_SDL -DHAVE_VORBIS -DHAVE_JWZGLES -DHAVE_ANDROID -DRENDERTYPESDL=1 -DUSE_OPENGL -DNETCODE_DISABLE -DUSE_LIBVPX \
                   -DHAVE_INTTYPES -D_GNU_SOURCE=1 -D_REENTRANT -D_NDK_MATH_NO_SOFTFP=1

LOCAL_LDFLAGS   := -fuse-ld=bfd
TARGET_LDFLAGS  += -Wl,--no-warn-mismatch -lm_hard
LOCAL_ARM_NEON  = true

ifeq ($(NDK_DEBUG), 1)
    COMMONFLAGS += -O0 -ggdb -fno-omit-frame-pointer -fno-stack-protector -D_FORTIFY_SOURCE=0 -DDEBUGGINGAIDS=0
else
    COMMONFLAGS += -O2 -DNDEBUG -DUSING_LTO -flto -D_FORTIFY_SOURCE=2
    LOCAL_LDFLAGS += -flto
endif

LOCAL_CFLAGS    = $(COMMONFLAGS)
LOCAL_CPPFLAGS  = $(COMMONFLAGS)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/source $(LOCAL_PATH)/source/jmact $(LOCAL_PATH)/source/jaudiolib/include $(LOCAL_PATH)/source/enet/include $(LOCAL_PATH)/build/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/source/android/etcpak
LOCAL_C_INCLUDES += $(TOP_DIR)/ $(TOP_DIR)/Libraries/liboggvorbis/include $(TOP_DIR)/Libraries/ $(TOP_DIR)/Libraries/SDL2/include $(TOP_DIR)/Libraries/SDL2_mixer/include $(TOP_DIR)/Libraries/TinyXML/include $(TOP_DIR)/TouchControls $(TOP_DIR)/Libraries/libvpx/include

ANDROID_SRC = \
    build/src/jwzgles.c \
    source/android/android-jni.cpp \
    source/android/etcpak/ProcessRGB.cpp \
    source/android/etcpak/Tables.cpp \
    source/android/in_android.c

BUILD_SRC = \
    build/src/a-c.c \
    build/src/baselayer.c \
    build/src/cache1d.c \
    build/src/compat.c \
    build/src/common.c \
    build/src/crc32.c \
    build/src/defs.c \
    build/src/colmatch.c \
    build/src/engine.c \
    build/src/glbuild.c \
    build/src/polymost.c \
    build/src/mdsprite.c \
    build/src/texcache.c \
    build/src/dxtfilter.c \
    build/src/hightile.c \
    build/src/textfont.c \
    build/src/smalltextfont.c \
    build/src/kplib.c \
    build/src/mmulti_null.c \
    build/src/lz4.c \
    build/src/osd.c \
    build/src/md4.c \
    build/src/pragmas.c \
    build/src/scriptfile.c \
    build/src/mutex.c \
    build/src/xxhash.c \
    build/src/voxmodel.c \
    build/src/sdlayer.c

JMACT_SRC = \
    source/jmact/file_lib.c \
    source/jmact/control.c \
    source/jmact/keyboard.c \
    source/jmact/mouse.c \
    source/jmact/joystick.c \
    source/jmact/scriplib.c \
    source/jmact/animlib.c

GAME_SRC = \
    source/game.c \
    source/actors.c \
    source/anim.c \
    source/animsounds.c \
    source/animvpx.c \
    source/common.c \
    source/config.c \
    source/demo.c \
    source/gamedef.c \
    source/gameexec.c \
    source/gamevars.c \
    source/global.c \
    source/input.c \
    source/menus.c \
    source/namesdyn.c \
    source/net.c \
    source/player.c \
    source/premap.c \
    source/savegame.c \
    source/sector.c \
    source/rts.c \
    source/osdfuncs.c \
    source/osdcmds.c \
    source/grpscan.c \
    source/sounds.c \
    source/soundsdyn.c \
    source/sdlmusic.c \
    source/rev.c \
    source/cmdline.c \
    source/screens.c \
    source/screentext.c \
    source/cheats.c \
    source/sbar.c

JAUDIO_SRC = \
    source/jaudiolib/src/drivers.c \
    source/jaudiolib/src/fx_man.c \
    source/jaudiolib/src/multivoc.c \
    source/jaudiolib/src/mix.c \
    source/jaudiolib/src/mixst.c \
    source/jaudiolib/src/pitch.c \
    source/jaudiolib/src/formats.c \
    source/jaudiolib/src/vorbis.c \
    source/jaudiolib/src/flac.c \
    source/jaudiolib/src/xa.c \
    source/jaudiolib/src/driver_nosound.c \
    source/jaudiolib/src/driver_sdl.c

ENET_SRC = \
    source/enet/src/callbacks.c \
    source/enet/src/host.c \
    source/enet/src/list.c \
    source/enet/src/packet.c \
    source/enet/src/peer.c \
    source/enet/src/protocol.c \
    source/enet/src/compress.c \
    source/enet/src/unix.c

LOCAL_SRC_FILES         = $(ANDROID_SRC) $(JAUDIO_SRC) $(JMACT_SRC) $(GAME_SRC) $(BUILD_SRC)

LOCAL_LDLIBS            := -lGLESv1_CM -lEGL -ldl -llog
LOCAL_STATIC_LIBRARIES  := touch
LOCAL_SHARED_LIBRARIES  := ogg vorbis SDL2 SDL2_mixer libvpx

include $(BUILD_SHARED_LIBRARY)
