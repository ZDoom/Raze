# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.crg/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := duke

# -O2  -fvisibility=hidden

LOCAL_CFLAGS :=   -x c++ -std=gnu++03 -fvisibility=hidden -fPIC -O2 -DNDEBUG -DUSING_LTO -flto -fno-stack-protector -funsigned-char -fno-strict-aliasing -DNO_GCC_BUILTINS -D_FORTIFY_SOURCE=0 -pthread -DHAVE_INTTYPES  -D_GNU_SOURCE=1 -D_REENTRANT
LOCAL_CFLAGS += -W  -Werror-implicit-function-declaration -Wpointer-arith -Wextra  -Wno-unused-result  -Wno-char-subscripts -Wno-strict-overflow -Wno-attributes -Wno-write-strings
LOCAL_CPPFLAGS := -std=gnu++03

LOCAL_CFLAGS += -DHAVE_SDL -DHAVE_VORBIS -DHAVE_JWZGLES -DHAVE_ANDROID -DRENDERTYPESDL=1  -DUSE_OPENGL -DNETCODE_DISABLE -DUSE_LIBVPX

#LOCAL_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1

LOCAL_LDFLAGS := -fuse-ld=bfd
LOCAL_ARM_NEON = true

LOCAL_C_INCLUDES :=  $(LOCAL_PATH)/source $(LOCAL_PATH)/source/jmact $(LOCAL_PATH)/source/jaudiolib/include $(LOCAL_PATH)/source/enet/include  $(LOCAL_PATH)/build/include
LOCAL_C_INCLUDES +=    $(TOP_DIR)/ $(TOP_DIR)/Libraries/liboggvorbis/include $(TOP_DIR)/Libraries/ $(TOP_DIR)/Libraries/SDL2/include  $(TOP_DIR)/Libraries/SDL2_mixer/include $(TOP_DIR)/Libraries/libpng/include   $(TOP_DIR)/Libraries/TinyXML/include $(TOP_DIR)/TouchControls $(TOP_DIR)/Libraries/libvpx/include

ANDROID_SRC = \
	source/android/android-jni.cpp \
	source/android/in_android.c \
        build/src/jwzgles.c \
        source/android/rg_etc1.cpp

BUILD_SRC = \
	build/src/a-c.c \
	build/src/baselayer.c \
	build/src/cache1d.c \
	build/src/compat.c \
	build/src/common.c \
	build/src/crc32.c \
	build/src/defs.c \
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

JMACT_SRC=source/jmact/file_lib.c \
	source/jmact//control.c \
	source/jmact//keyboard.c \
	source/jmact//mouse.c \
	source/jmact//joystick.c \
	source/jmact//scriplib.c \
	source/jmact//animlib.c

GAME_SRC=source/game.c \
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
  	source/rev.c 
 
 JAUDIO_SRC=source/jaudiolib/src/drivers.c \
	source/jaudiolib/src//fx_man.c \
	source/jaudiolib/src//multivoc.c \
	source/jaudiolib/src//mix.c \
	source/jaudiolib/src//mixst.c \
	source/jaudiolib/src//pitch.c \
	source/jaudiolib/src//formats.c \
	source/jaudiolib/src//vorbis.c \
	source/jaudiolib/src//flac.c \
	source/jaudiolib/src//xa.c \
	source/jaudiolib/src//driver_nosound.c \
	source/jaudiolib/src//driver_sdl.c
 
 
 ENET_SRC=source/enet/src/callbacks.c \
	source/enet/src/host.c \
	source/enet/src/list.c \
	source/enet/src/packet.c \
	source/enet/src/peer.c \
	source/enet/src/protocol.c \
	source/enet/src/compress.c \
    source/enet/src/unix.c
 
LOCAL_SRC_FILES = $(ANDROID_SRC) $(JAUDIO_SRC) $(JMACT_SRC) $(GAME_SRC) $(BUILD_SRC)  

LOCAL_LDLIBS :=  -lGLESv1_CM -lEGL -ldl -llog -lOpenSLES -lz -L$(TOP_DIR)/openssl/libs/ 
LOCAL_STATIC_LIBRARIES :=   libpng crypto 
LOCAL_SHARED_LIBRARIES := touchcontrols ogg vorbis SDL2 SDL2_mixer libvpx
# SDL2_image

ifeq ($(GP_LIC),1)
LOCAL_STATIC_LIBRARIES +=  s-setup
LOCAL_CFLAGS += -DGP_LIC
endif

include $(BUILD_SHARED_LIBRARY)






