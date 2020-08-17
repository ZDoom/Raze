#pragma once
#include <functional>
#include "dobject.h"
#include "v_2ddrawer.h"

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

class DBlackScreen : public DScreenJob
{
	int wait;

public:
	DBlackScreen(int w) : wait(w) {}
	int Frame(uint64_t clock, bool skiprequest) override;
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
	int waittime; // in ms.
	FGameTexture* tex = nullptr;

public:
	DImageScreen(FGameTexture* tile, int fade = DScreenJob::fadein | DScreenJob::fadeout, int wait = 3000) : DScreenJob(fade), waittime(wait)
	{
		tex = tile;
	}

	DImageScreen(int tile, int fade = DScreenJob::fadein | DScreenJob::fadeout, int wait = 3000) : DScreenJob(fade), waittime(wait)
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


void RunScreenJob(JobDesc *jobs, int count, CompletionFunc completion, bool clearbefore = true, bool blockingui = false);
void DeleteScreenJob();
void RunScreenJobFrame();

struct AnimSound
{
	int framenum;
	int soundnum;
};

DScreenJob *PlayVideo(const char *filename, const AnimSound *ans = nullptr, const int *frameticks = nullptr, bool nosoundstop = false);
