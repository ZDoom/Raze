/*
 Copyright (C) 2009 Jonathon Fowler <jf@jonof.id.au>
 
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 */


int NoSoundDrv_GetError(void);
const char *NoSoundDrv_ErrorString( int ErrorNumber );
int NoSoundDrv_PCM_Init(int *mixrate, int *numchannels, void * initdata);
void NoSoundDrv_PCM_Shutdown(void);
int NoSoundDrv_PCM_BeginPlayback(char *BufferStart, int BufferSize,
              int NumDivisions, void ( *CallBackFunc )( void ) );
void NoSoundDrv_PCM_StopPlayback(void);
void NoSoundDrv_PCM_Lock(void);
void NoSoundDrv_PCM_Unlock(void);
