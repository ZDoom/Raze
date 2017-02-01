//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------

#ifndef KEYS_H

#define KEYS_H

   #define NUM_CODES     128

   #define ESC    0x1B
   #define ENTER  0x0D

   #define KEYSC_ESC	  0x01
   #define KEYSC_1  	  0x02
   #define KEYSC_2  	  0x03
   #define KEYSC_3  	  0x04
   #define KEYSC_4  	  0x05
   #define KEYSC_5  	  0x06
   #define KEYSC_6  	  0x07
   #define KEYSC_7  	  0x08
   #define KEYSC_8  	  0x09
   #define KEYSC_9  	  0x0a
   #define KEYSC_0  	  0x0b
   #define KEYSC_DASH     0x0c
   #define KEYSC_EQUAL    0x0d

   #define KEYSC_BS 	  0x0e
   #define KEYSC_TAB	  0x0f
   #define KEYSC_Q  	  0x10
   #define KEYSC_W  	  0x11
   #define KEYSC_E  	  0x12
   #define KEYSC_R  	  0x13
   #define KEYSC_T  	  0x14
   #define KEYSC_Y  	  0x15
   #define KEYSC_U  	  0x16
   #define KEYSC_I  	  0x17
   #define KEYSC_O  	  0x18
   #define KEYSC_P  	  0x19
   #define KEYSC_LBRACK   0x1a
   #define KEYSC_RBRACK   0x1b
   #define KEYSC_ENTER    0x1c

   #define KEYSC_LCTRL    0x1d
   #define KEYSC_A  	  0x1e
   #define KEYSC_S  	  0x1f
   #define KEYSC_D  	  0x20
   #define KEYSC_F  	  0x21
   #define KEYSC_G  	  0x22
   #define KEYSC_H  	  0x23
   #define KEYSC_J  	  0x24
   #define KEYSC_K  	  0x25
   #define KEYSC_L  	  0x26
   #define KEYSC_SEMI     0x27
   #define KEYSC_QUOTE    0x28
   #define KEYSC_BQUOTE   0x29
   #define KEYSC_TILDE    0x29

   #define KEYSC_LSHIFT   0x2a
   #define KEYSC_BSLASH   0x2b
   #define KEYSC_Z  	  0x2c
   #define KEYSC_X  	  0x2d
   #define KEYSC_C  	  0x2e
   #define KEYSC_V  	  0x2f
   #define KEYSC_B  	  0x30
   #define KEYSC_N  	  0x31
   #define KEYSC_M  	  0x32
   #define KEYSC_COMMA    0x33
   #define KEYSC_PERIOD   0x34
   #define KEYSC_SLASH    0x35
   #define KEYSC_RSHIFT   0x36
   #define KEYSC_gSTAR    0x37

   #define KEYSC_LALT     0x38
   #define KEYSC_SPACE    0x39
   #define KEYSC_CAPS     0x3a

   #define KEYSC_F1 	  0x3b
   #define KEYSC_F2 	  0x3c
   #define KEYSC_F3 	  0x3d
   #define KEYSC_F4 	  0x3e
   #define KEYSC_F5 	  0x3f
   #define KEYSC_F6 	  0x40
   #define KEYSC_F7 	  0x41
   #define KEYSC_F8 	  0x42
   #define KEYSC_F9 	  0x43
   #define KEYSC_F10	  0x44

   #define KEYSC_gNUM     0x45
   #define KEYSC_SCROLL   0x46

   #define KEYSC_gHOME    0x47
   #define KEYSC_gUP	  0x48
   #define KEYSC_gPGUP    0x49
   #define KEYSC_gMINUS   0x4a
   #define KEYSC_gLEFT    0x4b
   #define KEYSC_gKP5     0x4c
   #define KEYSC_gRIGHT   0x4d
   #define KEYSC_gPLUS    0x4e
   #define KEYSC_gEND     0x4f
   #define KEYSC_gDOWN    0x50
   #define KEYSC_gPGDN    0x51
   #define KEYSC_gINS     0x52
   #define KEYSC_gDEL     0x53

   #define KEYSC_F11    0x57
   #define KEYSC_F12    0x58

   #define KEYSC_gENTER   0x9C
   #define KEYSC_RCTRL    0x9D
   #define KEYSC_gSLASH   0xB5
   #define KEYSC_RALT     0xB8
   #define KEYSC_PRTSCN   0xB7
   #define KEYSC_PAUSE    0xC5
   #define KEYSC_HOME     0xC7
   #define KEYSC_UP 	  0xC8
   #define KEYSC_PGUP     0xC9
   #define KEYSC_LEFT     0xCB
   #define KEYSC_RIGHT    0xCD
   #define KEYSC_END	  0xCF
   #define KEYSC_DOWN     0xD0
   #define KEYSC_PGDN     0xD1
   #define KEYSC_INSERT   0xD2
   #define KEYSC_DELETE   0xD3

    #define asc_Esc         27
    #define asc_Enter       13
    #define asc_Space       32

#endif
