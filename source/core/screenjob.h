#pragma once
#include <functional>
#include "dobject.h"

using CompletionFunc = std::function<void(bool)>;
struct JobDesc;
class ScreenJobRunner;

class DScreenJob : public DObject
{
	DECLARE_CLASS(DScreenJob, DObject)
	int64_t now;
	const int fadestyle;
	const float fadetime;	// in milliseconds
	int fadestate = fadein;

	friend class ScreenJobRunner;

public:
	enum
	{
		visible = 0,
		fadein = 1,
		fadeout = 2,
	};

	DScreenJob(int fade = 0, float fadet = 250.f) : fadestyle(fade), fadetime(fadet) {}

	void SetClock(int64_t nsnow)
	{
		now = nsnow;
	}

	int64_t GetClock() const
	{
		return now;
	}

	virtual int Frame(uint64_t clock, bool skiprequest) { return 0; }
	int GetFadeState() const { return fadestate; }

};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DImageScreen : public DScreenJob
{
	DECLARE_CLASS(DImageScreen, DScreenJob)

	int tilenum = -1;
	FGameTexture* tex = nullptr;

public:
	DImageScreen(FGameTexture* tile, int fade = DScreenJob::fadein | DScreenJob::fadeout) : DScreenJob(fade)
	{
		tex = tile;
	}

	DImageScreen(int tile, int fade = DScreenJob::fadein | DScreenJob::fadeout) : DScreenJob(fade)
	{
		tilenum = tile;
	}

	int Frame(uint64_t clock, bool skiprequest) override;
};




struct JobDesc
{
	DScreenJob* job;
	void (*postAction)();
	bool ignoreifskipped;
};


void RunScreenJob(JobDesc *jobs, int count, CompletionFunc completion, bool clearbefore = true);
void DeleteScreenJob();
void RunScreenJobFrame();

struct AnimSound
{
	int framenum;
	int soundnum;
};

DScreenJob *PlayVideo(const char *filename, const AnimSound *ans = nullptr, const int *frameticks = nullptr);
