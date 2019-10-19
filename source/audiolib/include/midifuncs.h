/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef __MIDIFUNCS_H
#define __MIDIFUNCS_H

typedef struct
   {
   void ( *NoteOff )( int channel, int key, int velocity );
   void ( *NoteOn )( int channel, int key, int velocity );
   void ( *PolyAftertouch )( int channel, int key, int pressure );
   void ( *ControlChange )( int channel, int number, int value );
   void ( *ProgramChange )( int channel, int program );
   void ( *ChannelAftertouch )( int channel, int pressure );
   void ( *PitchBend )( int channel, int lsb, int msb );
   void ( *ReleasePatches )( void );
   void ( *LoadPatch )( int number );
   void ( *SetVolume )( int volume );
   int  ( *GetVolume )( void );
   void ( *SysEx )( const unsigned char * data, int length );
   } midifuncs;

#endif
