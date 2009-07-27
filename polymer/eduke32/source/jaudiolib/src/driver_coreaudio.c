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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 
 */

/**
 * CoreAudio output driver for MultiVoc
 *
 * Inspired by the example set by the Audiere sound library available at
 *   https://audiere.svn.sourceforge.net/svnroot/audiere/trunk/audiere/
 *
 */

#include <AudioUnit/AudioUnit.h>
#include <pthread.h>
#include "driver_coreaudio.h"

enum {
   CAErr_Warning = -2,
   CAErr_Error   = -1,
   CAErr_Ok      = 0,
	CAErr_Uninitialised,
	CAErr_FindNextComponent,
	CAErr_OpenAComponent,
	CAErr_AudioUnitInitialize,
	CAErr_AudioUnitSetProperty,
	CAErr_Mutex
};

static int32_t ErrorCode = CAErr_Ok;
static int32_t Initialised = 0;
static int32_t Playing = 0;
static ComponentInstance output_audio_unit;
static pthread_mutex_t mutex;

static char *MixBuffer = 0;
static int32_t MixBufferSize = 0;
static int32_t MixBufferCount = 0;
static int32_t MixBufferCurrent = 0;
static int32_t MixBufferUsed = 0;
static void ( *MixCallBack )( void ) = 0;

static OSStatus fillInput(
					void                         *inRefCon,
					AudioUnitRenderActionFlags   inActionFlags,
					const AudioTimeStamp         *inTimeStamp,
					UInt32                       inBusNumber,
					AudioBuffer                  *ioData)
{
	UInt32 remaining, len;
	char *ptr, *sptr;
	
	remaining = ioData->mDataByteSize;
	ptr = ioData->mData;
	
	while (remaining > 0) {
		if (MixBufferUsed == MixBufferSize) {
			CoreAudioDrv_Lock();
			MixCallBack();
			CoreAudioDrv_Unlock();
			
			MixBufferUsed = 0;
			MixBufferCurrent++;
			if (MixBufferCurrent >= MixBufferCount) {
				MixBufferCurrent -= MixBufferCount;
			}
		}
		
		while (remaining > 0 && MixBufferUsed < MixBufferSize) {
			sptr = MixBuffer + (MixBufferCurrent * MixBufferSize) + MixBufferUsed;
			
			len = MixBufferSize - MixBufferUsed;
			if (remaining < len) {
				len = remaining;
			}
			
			memcpy(ptr, sptr, len);
			
			ptr += len;
			MixBufferUsed += len;
			remaining -= len;
		}
	}

	return noErr;
}

int32_t CoreAudioDrv_GetError(void)
{
	return ErrorCode;
}

const char *CoreAudioDrv_ErrorString( int32_t ErrorNumber )
{
	const char *ErrorString;
	
   switch( ErrorNumber )
	{
      case CAErr_Warning :
      case CAErr_Error :
         ErrorString = CoreAudioDrv_ErrorString( ErrorCode );
         break;
			
      case CAErr_Ok :
         ErrorString = "CoreAudio ok.";
         break;
			
		case CAErr_Uninitialised:
			ErrorString = "CoreAudio uninitialised.";
			break;
			
		case CAErr_FindNextComponent:
			ErrorString = "CoreAudio error: FindNextComponent returned NULL.";
			break;
			
		case CAErr_OpenAComponent:
			ErrorString = "CoreAudio error: OpenAComponent failed.";
			break;
			
		case CAErr_AudioUnitInitialize:
			ErrorString = "CoreAudio error: AudioUnitInitialize failed.";
			break;
			
		case CAErr_AudioUnitSetProperty:
			ErrorString = "CoreAudio error: AudioUnitSetProperty failed.";
			break;

		case CAErr_Mutex:
			ErrorString = "CoreAudio error: could not initialise mutex.";
			break;
			
		default:
			ErrorString = "Unknown CoreAudio error code.";
			break;
	}
	
	return ErrorString;
}

/*
int32_t CoreAudioDrv_InitMIDI()
{
    AudioStreamBasicDescription requestedDesc;
    
    requestedDesc.mFormatID = kAudioFormatMIDIStream;
}
 */

int32_t CoreAudioDrv_Init(int32_t mixrate, int32_t numchannels, int32_t samplebits, void * initdata)
{
	OSStatus result = noErr;
	Component comp;
	ComponentDescription desc;
	AudioStreamBasicDescription requestedDesc;
	struct AudioUnitInputCallback callback;
	
	if (Initialised) {
		CoreAudioDrv_Shutdown();
	}
	
	if (pthread_mutex_init(&mutex, 0) < 0) {
		ErrorCode = CAErr_Mutex;
		return CAErr_Error;
	}

   // Setup a AudioStreamBasicDescription with the requested format
	requestedDesc.mFormatID = kAudioFormatLinearPCM;
	requestedDesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
	requestedDesc.mChannelsPerFrame = numchannels;
	requestedDesc.mSampleRate = mixrate;
	
	requestedDesc.mBitsPerChannel = samplebits;
	if (samplebits == 16) {
		requestedDesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
#ifdef __POWERPC__
		requestedDesc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
#endif
	}
	
	requestedDesc.mFramesPerPacket = 1;
	requestedDesc.mBytesPerFrame = requestedDesc.mBitsPerChannel * \
		requestedDesc.mChannelsPerFrame / 8;
	requestedDesc.mBytesPerPacket = requestedDesc.mBytesPerFrame * \
		requestedDesc.mFramesPerPacket;
	
   // Locate the default output audio unit
	desc.componentType = kAudioUnitComponentType;
	desc.componentSubType = kAudioUnitSubType_Output;
	desc.componentManufacturer = kAudioUnitID_DefaultOutput;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	comp = FindNextComponent(NULL, &desc);
	if (comp == NULL) {
      ErrorCode = CAErr_FindNextComponent;
		pthread_mutex_destroy(&mutex);
      return CAErr_Error;
	}
	
    // Open & initialize the default output audio unit
	result = OpenAComponent(comp, &output_audio_unit);
	if (result != noErr) {
      ErrorCode = CAErr_OpenAComponent;
      CloseComponent(output_audio_unit);
		pthread_mutex_destroy(&mutex);
      return CAErr_Error;
	}
	
	result = AudioUnitInitialize(output_audio_unit);
	if (result != noErr) {
      ErrorCode = CAErr_AudioUnitInitialize;
      CloseComponent(output_audio_unit);
		pthread_mutex_destroy(&mutex);
      return CAErr_Error;
	}
	
    // Set the input format of the audio unit.
	result = AudioUnitSetProperty(output_audio_unit,
											kAudioUnitProperty_StreamFormat,
											kAudioUnitScope_Input,
											0,
											&requestedDesc,
											sizeof(requestedDesc));
	if (result != noErr) {
      ErrorCode = CAErr_AudioUnitSetProperty;
      CloseComponent(output_audio_unit);
		pthread_mutex_destroy(&mutex);
      return CAErr_Error;
	}
	
    // Set the audio callback
	callback.inputProc = fillInput;
	callback.inputProcRefCon = 0;
	AudioUnitSetProperty(output_audio_unit,
								kAudioUnitProperty_SetInputCallback,
								kAudioUnitScope_Input,
								0,
								&callback,
								sizeof(callback));
	
	Initialised = 1;
	
	return CAErr_Ok;
}

void CoreAudioDrv_Shutdown(void)
{
	OSStatus result;
	struct AudioUnitInputCallback callback;

	if (!Initialised) {
		return;
	}
	
    // stop processing the audio unit
	CoreAudioDrv_StopPlayback();
	
    // Remove the input callback
	callback.inputProc = 0;
	callback.inputProcRefCon = 0;
	result = AudioUnitSetProperty(output_audio_unit,
											kAudioUnitProperty_SetInputCallback,
											kAudioUnitScope_Input,
											0,
											&callback,
											sizeof(callback));
	result = CloseComponent(output_audio_unit);

	pthread_mutex_destroy(&mutex);
	
	Initialised = 0;
}

int32_t CoreAudioDrv_BeginPlayback(char *BufferStart, int32_t BufferSize,
									  int32_t NumDivisions, void ( *CallBackFunc )( void ) )
{
	if (!Initialised) {
		ErrorCode = CAErr_Uninitialised;
		return CAErr_Error;
	}
	
	if (Playing) {
		CoreAudioDrv_StopPlayback();
	}
	
	MixBuffer = BufferStart;
	MixBufferSize = BufferSize;
	MixBufferCount = NumDivisions;
	MixBufferCurrent = 0;
	MixBufferUsed = 0;
	MixCallBack = CallBackFunc;
	
	// prime the buffer
	MixCallBack();
	
	AudioOutputUnitStart(output_audio_unit);
	
	Playing = 1;
	
	return CAErr_Ok;
}

void CoreAudioDrv_StopPlayback(void)
{
	if (!Initialised || !Playing) {
		return;
	}
	
	CoreAudioDrv_Lock();
	AudioOutputUnitStop(output_audio_unit);
	CoreAudioDrv_Unlock();
	
	Playing = 0;
}

void CoreAudioDrv_Lock(void)
{
	pthread_mutex_lock(&mutex);
}

void CoreAudioDrv_Unlock(void)
{
	pthread_mutex_unlock(&mutex);
}
