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

LOCAL_CFLAGS :=  -fvisibility=hidden -fPIC -Wimplicit -O2 -funswitch-loops -fomit-frame-pointer -DNDEBUG -DUSING_LTO -flto -fno-stack-protector   -W  -Werror-implicit-function-declaration -Wpointer-arith -Wextra  -funsigned-char -fno-strict-aliasing -D_FORTIFY_SOURCE=0 -fjump-tables -Wno-unused-result  -Wno-char-subscripts    -pthread -DHAVE_INTTYPES  -D_GNU_SOURCE=1 -D_REENTRANT -DRENDERTYPESDL=1 -Wno-strict-overflow -DUSE_OPENGL  -Wno-attributes

#-DUSE_LIBPNG

LOCAL_CFLAGS += -DHAVE_SDL -DHAVE_VORBIS -DDROIDMENU

#LOCAL_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1

LOCAL_ARM_NEON = true

LOCAL_LDLIBS += -lGLESv1_CM -lEGL

LOCAL_LDLIBS += -llog

LOCAL_C_INCLUDES :=  $(LOCAL_PATH)/source $(LOCAL_PATH)/source/jmact $(LOCAL_PATH)/source/jaudiolib/include $(LOCAL_PATH)/source/enet/include  $(LOCAL_PATH)/build/include

LOCAL_C_INCLUDES +=    $(TOP_DIR)/ $(TOP_DIR)/Libraries/liboggvorbis/include $(TOP_DIR)/Libraries/ $(TOP_DIR)/Libraries/SDL2/include  $(TOP_DIR)/Libraries/SDL2_mixer/include $(TOP_DIR)/Libraries/libpng/include   $(TOP_DIR)/Libraries/TinyXML/include $(TOP_DIR)/TouchControls 

ANDROID_SRC = \
	source/android/android-jni.cpp \
	source/android/in_android.c

BUILD_SRC = \
	build/src/a-c.c \
	build/src/baselayer.c \
	build/src/cache1d.c \
	build/src/compat.c \
	build/src/crc32.c \
	build/src/defs.c \
	build/src/engine.c \
	build/src/polymost.c \
	build/src/texcache.c \
	build/src/dxtfilter.c \
	build/src/hightile.c \
	build/src/textfont.c \
	build/src/smalltextfont.c \
	build/src/kplib.c \
	build/src/lz4.c \
	build/src/osd.c \
	build/src/pragmas.c \
	build/src/scriptfile.c \
	build/src/mutex.c \
    build/src/xxhash.c \
    build/src/mmulti_null.c \
    build/src/voxmodel.c \
    build/src/common.c \
    

GL_SRC = \
 	build/src/mdsprite.c \
 	build/src/glbuild_android.c \

SDL_SRC = \
	build/src/sdlayer.c \

JMACT_SRC=source/jmact/file_lib.c \
	source/jmact//control.c \
	source/jmact//keyboard.c \
	source/jmact//mouse.c \
	source/jmact//joystick.c \
	source/jmact//mathutil.c \
	source/jmact//scriplib.c \
	source/jmact//animlib.c

GAME_SRC=source/game.c \
	source/actors.c \
	source/anim.c \
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
 
LOCAL_SRC_FILES = $(ANDROID_SRC) $(ENET_SRC) $(JAUDIO_SRC) $(JMACT_SRC) $(GAME_SRC) $(BUILD_SRC)  $(GL_SRC) $(SDL_SRC)  


LOCAL_LDLIBS :=  -lGLESv1_CM -lEGL -ldl -llog -lOpenSLES -lz -L$(TOP_DIR)/openssl/libs/ -lcrypto
LOCAL_STATIC_LIBRARIES :=  nanogl  SDL2_net libjpeg libpng
LOCAL_SHARED_LIBRARIES := touchcontrols openal ogg vorbis SDL2 SDL2_mixer SDL2_image

ifeq ($(GP_LIC),1)
LOCAL_STATIC_LIBRARIES +=  s-setup
LOCAL_CFLAGS += -DGP_LIC
endif

include $(BUILD_SHARED_LIBRARY)






