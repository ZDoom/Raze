/*
** movieplayer.cpp
**
**---------------------------------------------------------------------------
** Copyright 2020 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "types.h"
#include "build.h"
#include "screenjob.h"
#include "i_time.h"
#include "v_2ddrawer.h"
#include "animlib.h"
#include "v_draw.h"
#include "s_soundinternal.h"
#include "animtexture.h"
#include "gamestate.h"
#include "razemenu.h"
#include "raze_sound.h"
#include "SmackerDecoder.h"
#include "movie/playmve.h"
#include "gamecontrol.h"
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include "raze_music.h"


class MoviePlayer
{
public:
	virtual void Start() {}
	virtual bool Frame(uint64_t clock) = 0;
	virtual void Stop() {}
	virtual ~MoviePlayer() = default;
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class AnmPlayer : public MoviePlayer
{
	// This doesn't need its own class type
	anim_t anim;
	TArray<uint8_t> buffer;
	int numframes = 0;
	int curframe = 1;
	int frametime = 0;
	int nextframetime = 0;
	AnimTextures animtex;
	const AnimSound* animSnd;
	const int* frameTicks;
	bool nostopsound;

public:
	bool isvalid() { return numframes > 0; }

	AnmPlayer(FileReader& fr, const AnimSound* ans, const int *frameticks, bool nosoundcutoff)
		: animSnd(ans), frameTicks(frameticks), nostopsound(nosoundcutoff)
	{
		buffer = fr.ReadPadded(1);
		fr.Close();

		if (ANIM_LoadAnim(&anim, buffer.Data(), buffer.Size() - 1) < 0)
		{
			return;
		}
		numframes = ANIM_NumFrames(&anim);
		animtex.SetSize(AnimTexture::Paletted, 320, 200);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	bool Frame(uint64_t clock) override
	{
		int currentclock = int(clock * 120 / 1'000'000'000);

		if (currentclock < nextframetime - 1)
		{
			twod->ClearScreen();
			DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Masked, false, TAG_DONE);
			return true;
		}

		animtex.SetFrame(ANIM_GetPalette(&anim), ANIM_DrawFrame(&anim, curframe));
		frametime = currentclock;

		twod->ClearScreen();
		DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, DTA_Masked, false, TAG_DONE);

		int delay = 20;
		if (frameTicks)
		{
			if (curframe == 1) delay = frameTicks[0];
			else if (curframe < numframes - 2) delay = frameTicks[1];
			else delay = frameTicks[2];
		}
		nextframetime += delay;

		if (animSnd) for (int i = 0; animSnd[i].framenum >= 0; i++)
		{
			if (animSnd[i].framenum == curframe)
			{
				int sound = animSnd[i].soundnum;
				if (sound == -1)
					soundEngine->StopAllChannels();
				else if (SoundEnabled())
					soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI, sound, 1.f, ATTN_NONE);
			}
		}
		if (!nostopsound && curframe == numframes && soundEngine->GetSoundPlayingInfo(SOURCE_None, nullptr, -1)) return true;
		curframe++;
		return curframe < numframes;
	}

	void Stop() override
	{
		if (!nostopsound) soundEngine->StopAllChannels();
	}


	~AnmPlayer()
	{
		buffer.Reset();
		animtex.Clean();
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class MvePlayer : public MoviePlayer
{
	InterplayDecoder decoder;
	bool failed = false;
	
public:
	bool isvalid() { return !failed; }

	MvePlayer(FileReader& fr) : decoder(SoundEnabled())
	{
		failed = !decoder.Open(fr);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	bool Frame(uint64_t clock) override
	{
		if (failed) return false;
		bool playon = decoder.RunFrame(clock);
		twod->ClearScreen();
		DrawTexture(twod, decoder.animTex().GetFrame(), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
		return playon;
	}

	~MvePlayer()
	{
		decoder.Close();
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class VpxPlayer : public MoviePlayer
{
	bool failed = false;
	FileReader fr;
	AnimTextures animtex;
	const AnimSound* animSnd;

	unsigned width, height;
	TArray<uint8_t> Pic;
	TArray<uint8_t> readBuf;
	vpx_codec_ctx_t codec{};
	vpx_codec_iter_t iter = nullptr;

	uint32_t convnumer;
	uint32_t convdenom;

	uint64_t nsecsperframe;
	uint64_t nextframetime;

	int decstate = 0;
	int framenum = 0;
	int numframes;
	int lastsoundframe = -1;
public:
	int soundtrack = -1;


public:
	bool isvalid() { return !failed; }

	VpxPlayer(FileReader& fr_, const AnimSound* animSnd_, int origframedelay, FString& error)
	{
		fr = std::move(fr_);
		animSnd = animSnd_;

		if (!ReadIVFHeader(origframedelay))
		{
			// We should never get here, because any file failing this has been eliminated before this constructor got called.
			error.Format("Failed reading IVF header\n");
			failed = true;
		}

		Pic.Resize(width * height * 4);


		// Todo: Support VP9 as well?
		vpx_codec_dec_cfg_t cfg = { 1, width, height };
		if (vpx_codec_dec_init(&codec, &vpx_codec_vp8_dx_algo, &cfg, 0))
		{
			error.Format("Error initializing VPX codec.\n");
			failed = true;
		}
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	bool ReadIVFHeader(int origframedelay)
	{
		// IVF format: http://wiki.multimedia.cx/index.php?title=IVF
		uint32_t magic; fr.Read(&magic, 4); // do not byte swap!
		if (magic != MAKE_ID('D', 'K', 'I', 'F')) return false;
		uint16_t version = fr.ReadUInt16();
		if (version != 0) return false;
		uint16_t length = fr.ReadUInt16();
		if (length != 32) return false;
		fr.Read(&magic, 4);
		if (magic != MAKE_ID('V', 'P', '8', '0')) return false;

		width = fr.ReadUInt16();
		height = fr.ReadUInt16();
		uint32_t fpsdenominator = fr.ReadUInt32();
		uint32_t fpsnumerator = fr.ReadUInt32();
		numframes = fr.ReadUInt32();
		if (numframes == 0) return false;
		fr.Seek(4, FileReader::SeekCur);

		if (fpsdenominator > 1000 || fpsnumerator == 0 || fpsdenominator == 0)
		{
			// default to 30 fps if the header does not provide useful info.
			fpsdenominator = 30;
			fpsnumerator = 1;
		}

		convnumer = 120 * fpsnumerator;
		convdenom = fpsdenominator * origframedelay;

		nsecsperframe = int64_t(fpsnumerator) * 1'000'000'000 / fpsdenominator;
		nextframetime = 0;

		return true;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	bool ReadFrame()
	{
		int corrupted = 0;
		int framesize = fr.ReadInt32();
		fr.Seek(8, FileReader::SeekCur);
		if (framesize == 0) return false;

		readBuf.Resize(framesize);
		if (fr.Read(readBuf.Data(), framesize) != framesize) return false;
		if (vpx_codec_decode(&codec, readBuf.Data(), readBuf.Size(), NULL, 0) != VPX_CODEC_OK) return false;
		if (vpx_codec_control(&codec, VP8D_GET_FRAME_CORRUPTED, &corrupted) != VPX_CODEC_OK) return false;
		return true;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	vpx_image_t *GetFrameData()
	{
		vpx_image_t *img;
		do
		{
			if (decstate == 0)  // first time / begin
			{
				if (!ReadFrame()) return nullptr;
				decstate = 1;
			}

			img = vpx_codec_get_frame(&codec, &iter);
			if (img == nullptr)
			{
				decstate = 0;
				iter = nullptr;
			}
		} while (img == nullptr);

		return img->d_w == width && img->d_h == height? img : nullptr;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void SetPixel(uint8_t* dest, uint8_t y, uint8_t u, uint8_t v)
	{
		dest[0] = y;
		dest[1] = u;
		dest[2] = v;
	}

	bool CreateNextFrame()
	{
		auto img = GetFrameData();
		if (!img) return false;
		uint8_t const* const yplane = img->planes[VPX_PLANE_Y];
		uint8_t const* const uplane = img->planes[VPX_PLANE_U];
		uint8_t const* const vplane = img->planes[VPX_PLANE_V];

		const int ystride = img->stride[VPX_PLANE_Y];
		const int ustride = img->stride[VPX_PLANE_U];
		const int vstride = img->stride[VPX_PLANE_V];

		for (unsigned int y = 0; y < height; y += 2)
		{
			unsigned int y1 = y + 1;
			unsigned int wy = width * y;
			unsigned int wy1 = width * y1;

			for (unsigned int x = 0; x < width; x += 2)
			{
				uint8_t u = uplane[ustride * (y >> 1) + (x >> 1)];
				uint8_t v = vplane[vstride * (y >> 1) + (x >> 1)];

				SetPixel(&Pic[(wy + x) << 2], yplane[ystride * y + x], u, v);
				SetPixel(&Pic[(wy + x + 1) << 2], yplane[ystride * y + x + 1], u, v);
				SetPixel(&Pic[(wy1 + x) << 2], yplane[ystride * y1 + x], u, v);
				SetPixel(&Pic[(wy1 + x + 1) << 2], yplane[ystride * y1 + x + 1], u, v);
			}
		}
		return true;
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	void Start() override
	{
		if (soundtrack > 0)
		{
			Mus_Play(nullptr, fileSystem.GetFileFullName(soundtrack, false), false);
		}
		animtex.SetSize(AnimTexture::YUV, width, height);
	}

	//---------------------------------------------------------------------------
	//
	// 
	//
	//---------------------------------------------------------------------------

	bool Frame(uint64_t clock) override
	{
		bool stop = false;
		if (clock > nextframetime)
		{
			nextframetime += nsecsperframe;

			if (!CreateNextFrame())
			{
				Printf(PRINT_BOLD, "Failed reading next frame\n");
				stop = true;
			}
			else
			{
				animtex.SetFrame(nullptr, Pic.Data());
			}
			framenum++;
			if (framenum >= numframes) stop = true;

			int soundframe = convdenom ? Scale(framenum, convnumer, convdenom) : framenum;
			if (soundframe > lastsoundframe)
			{
				if (animSnd && soundtrack == -1) for (int i = 0; animSnd[i].framenum >= 0; i++)
				{
					if (animSnd[i].framenum == soundframe)
					{
						int sound = animSnd[i].soundnum;
						if (sound == -1)
							soundEngine->StopAllChannels();
						else if (SoundEnabled())
							soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI, sound, 1.f, ATTN_NONE);
					}
				}
				lastsoundframe = soundframe;
			}
		}
		DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit, TAG_DONE);
		return !stop;
	}

	void Stop()
	{
		Mus_Stop();
	}

	~VpxPlayer()
	{
		vpx_codec_destroy(&codec);
		animtex.Clean();
	}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

struct AudioData
{
	int hFx;
	SmackerAudioInfo inf;

	int16_t samples[6000 * 20]; // must be a multiple of the stream buffer size and larger than the initial chunk of audio

	int nWrite;
	int nRead;
};

class SmkPlayer : public MoviePlayer
{
	SmackerHandle hSMK{};
	int numAudioTracks;
	AudioData adata;
	uint32_t nWidth, nHeight;
	uint8_t palette[768];
	AnimTextures animtex;
	TArray<uint8_t> pFrame;
	TArray<uint8_t> audioBuffer;
	int nFrameRate;
	int nFrames;
	bool fullscreenScale;
	uint64_t nFrameNs;
	int nFrame = 0;
	const AnimSound* animSnd;
	FString filename;
	SoundStream* stream = nullptr;

public:
	bool isvalid() { return hSMK.isValid; }

	static bool StreamCallbackFunc(SoundStream* stream, void* buff, int len, void* userdata)
	{
		SmkPlayer* pId = (SmkPlayer*)userdata;
		memcpy(buff, &pId->adata.samples[pId->adata.nRead], len);
		pId->adata.nRead += len / 2;
		if (pId->adata.nRead >= countof(pId->adata.samples)) pId->adata.nRead = 0;
		return true;
	}

	void copy8bitSamples(unsigned count)
	{
		for (unsigned i = 0; i < count; i++)
		{
			adata.samples[adata.nWrite] = (audioBuffer[i] - 128) << 8;
			if (++adata.nWrite >= countof(adata.samples)) adata.nWrite = 0;
		}
	}

	void copy16bitSamples(unsigned count)
	{
		auto ptr = (uint16_t*)audioBuffer.Data();
		for (unsigned i = 0; i < count/2; i++)
		{
			adata.samples[adata.nWrite] = *ptr++;
			if (++adata.nWrite >= countof(adata.samples)) adata.nWrite = 0;
		}
	}


	SmkPlayer(const char *fn, const AnimSound* ans, bool fixedviewport)
	{
		hSMK = Smacker_Open(fn);
		if (!hSMK.isValid)
		{
			return;
		}
		Smacker_GetFrameSize(hSMK, nWidth, nHeight);
		pFrame.Resize(nWidth * nHeight + std::max(nWidth, nHeight));
		nFrameRate = Smacker_GetFrameRate(hSMK);
		nFrameNs = 1'000'000'000 / nFrameRate;
		nFrames = Smacker_GetNumFrames(hSMK);
		Smacker_GetPalette(hSMK, palette);
		fullscreenScale = (!fixedviewport || (nWidth <= 320 && nHeight <= 200) || nWidth >= 640 || nHeight >= 480);

		bool hassound = false;
		numAudioTracks = Smacker_GetNumAudioTracks(hSMK);
		if (numAudioTracks)
		{
			adata.nWrite = 0;
			adata.nRead = 0;
			adata.inf = Smacker_GetAudioTrackDetails(hSMK, 0);
			if (adata.inf.idealBufferSize > 0)
			{
				audioBuffer.Resize(adata.inf.idealBufferSize);
				auto read = Smacker_GetAudioData(hSMK, 0, (int16_t*)audioBuffer.Data());
				if (adata.inf.bitsPerSample == 8) copy8bitSamples(read);
				else copy16bitSamples(read);
				animSnd = nullptr;
				hassound = true;
			}
		}
		if (!hassound)
		{
			adata.inf = {};
			animSnd = ans;
		}

	}

	//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

	void Start() override
	{
		animtex.SetSize(AnimTexture::Paletted, nWidth, nHeight);
	}

	//---------------------------------------------------------------------------
	//
	//
	//
	//---------------------------------------------------------------------------

	bool Frame(uint64_t clock) override
	{
		int frame = clock / nFrameNs;

		twod->ClearScreen();
		Printf("clock = %llu, frame = %d\n", clock, frame);
		if (frame > nFrame)
		{
			Printf("Updating\n");
			Smacker_GetPalette(hSMK, palette);
			Smacker_GetFrame(hSMK, pFrame.Data());
			animtex.SetFrame(palette, pFrame.Data());
			if (numAudioTracks)
			{
				auto read = Smacker_GetAudioData(hSMK, 0, (int16_t*)audioBuffer.Data());
				if (adata.inf.bitsPerSample == 8) copy8bitSamples(read);
				else copy16bitSamples(read);
				if (!stream && read) // the sound may not start in the first frame, but the stream cannot start without any sound data present.
					stream = S_CreateCustomStream(6000, adata.inf.sampleRate, adata.inf.nChannels, StreamCallbackFunc, this);

			}

		}
		if (fullscreenScale)
		{
			DrawTexture(twod, animtex.GetFrame(), 0, 0, DTA_FullscreenEx, FSMode_ScaleToFit43, TAG_DONE);
		}
		else
		{
			DrawTexture(twod, animtex.GetFrame(), 320, 240, DTA_VirtualWidth, 640, DTA_VirtualHeight, 480, DTA_CenterOffset, true, TAG_DONE);
		}
		if (frame > nFrame)
		{
			nFrame++;
			Smacker_GetNextFrame(hSMK);
			if (animSnd) for (int i = 0; animSnd[i].framenum >= 0; i++)
			{
				if (animSnd[i].framenum == nFrame)
				{
					int sound = animSnd[i].soundnum;
					if (sound == -1)
						soundEngine->StopAllChannels();
					else if (SoundEnabled())
						soundEngine->StartSound(SOURCE_None, nullptr, nullptr, CHAN_AUTO, CHANF_UI, sound, 1.f, ATTN_NONE);
				}
			}
		}

		return nFrame < nFrames;
	}

	~SmkPlayer()
	{
		Smacker_Close(hSMK);
		if (stream) S_StopCustomStream(stream);
		soundEngine->StopAllChannels();
		animtex.Clean();
	}
};

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DMoviePlayer : public DSkippableScreenJob
{
	MoviePlayer* player;
	bool started = false;

public:
	DMoviePlayer(MoviePlayer* mp)
	{
		player = mp;
		pausable = false;
	}


	void Draw(double smoothratio) override
	{
		if (!player)
		{
			state = stopped;
			return;
		}
		if (!started)
		{
			started = true;
			player->Start();
		}
		uint64_t clock = (ticks + smoothratio) * 1'000'000'000. / GameTicRate;
		if (state == running  && !player->Frame(clock))
		{
			state = finished;
		}
	}

	void OnDestroy() override
	{
		if (player)
		{
			player->Stop();
			delete player;
		}
		player = nullptr;
	}
};


//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

MoviePlayer* OpenMovie(const char* filename, const AnimSound* ans, const int* frameticks, bool nosoundcutoff, FString& error)
{
	FileReader fr;
	// first try as .ivf - but only if sounds are provided - the decoder is video only.
	if (ans)
	{
		auto fn = StripExtension(filename);
		DefaultExtension(fn, ".ivf");
		fr = fileSystem.OpenFileReader(fn);
	}

	if (!fr.isOpen()) fr = fileSystem.OpenFileReader(filename);
	if (!fr.isOpen())
	{
		int nLen = strlen(filename);
		// Strip the drive letter and retry.
		if (nLen >= 3 && isalpha(filename[0]) && filename[1] == ':' && filename[2] == '/')
		{
			filename += 3;
			fr = fileSystem.OpenFileReader(filename);
		}
		if (!fr.isOpen())
		{
			error.Format("%s: Unable to open video\n", filename);
			return nullptr;
		}
	}
	char id[20] = {};

	fr.Read(&id, 20);
	fr.Seek(-20, FileReader::SeekCur);

	if (!memcmp(id, "LPF ", 4))
	{
		auto anm = new AnmPlayer(fr, ans, frameticks, nosoundcutoff);
		if (!anm->isvalid())
		{
			error.Format("%s: invalid ANM file.\n", filename);
			delete anm;
			return nullptr;
		}
		return anm;
	}
	else if (!memcmp(id, "SMK2", 4))
	{
		fr.Close();
		auto anm = new SmkPlayer(filename, ans, true); // Fixme: Handle Blood's video scaling behavior more intelligently.
		if (!anm->isvalid())
		{
			error.Format("%s: invalid SMK file.\n", filename);
			delete anm;
			return nullptr;
		}
		return anm;
	}
	else if (!memcmp(id, "Interplay MVE File", 18))
	{
		auto anm = new MvePlayer(fr);
		if (!anm->isvalid())
		{
			delete anm;
			return nullptr;
		}
		return anm;
	}
	else if (!memcmp(id, "DKIF\0\0 \0VP80", 12))
	{
		auto anm = new VpxPlayer(fr, ans, frameticks ? frameticks[1] : 0, error);
		if (!anm->isvalid())
		{
			delete anm;
			return nullptr;
		}
		anm->soundtrack = LookupMusic(filename, true);
		return anm;
	}
	// add more formats here.
	else
	{
		error.Format("%s: Unknown video format\n", filename);
		return nullptr;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

DScreenJob* PlayVideo(const char* filename, const AnimSound* ans, const int* frameticks, bool nosoundcutoff)
{
	if (!filename)
	{
		return Create<DBlackScreen>(1);
	}
	FString error;
	auto movie = OpenMovie(filename, ans, frameticks, nosoundcutoff, error);
	if (!movie)
	{
		Printf(TEXTCOLOR_YELLOW, "%s", error.GetChars());
		return Create<DBlackScreen>(1);
	}
	return Create<DMoviePlayer>(movie);
}


