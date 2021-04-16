#pragma once
#include <functional>
#include "dobject.h"
#include "v_2ddrawer.h"
#include "d_eventbase.h"

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
protected:
	int ticks = 0;
	int state = running;
	bool pausable = true;

public:
	enum
	{
		running = 1,	// normal operation
		skipped = 2,	// finished by user skipping
		finished = 3,	// finished by completing its sequence
		stopping = 4,	// running ending animations / fadeout, etc. Will not accept more input.
		stopped = 5,	// we're done here.
	};
	enum
	{
		visible = 0,
		fadein = 1,
		fadeout = 2,
	};

	DScreenJob(int fade = 0, float fadet = 250.f) : fadestyle(fade), fadetime(fadet) {}
	
	virtual bool ProcessInput()
	{
		return false;
	}

	void SetClock(int64_t nsnow)
	{
		now = nsnow;
	}

	int64_t GetClock() const
	{
		return now;
	}

	virtual bool OnEvent(event_t* evt) { return false; }
	virtual void OnTick() { /*state = finished;*/ }
	virtual int Frame(uint64_t clock, bool skiprequest) { return 1; }
	virtual void Draw(double smoothratio) {}

	int Frame(uint64_t clock, bool skiprequest, double smoothratio)
	{
		if (state != running) smoothratio = 1; // this is necessary because the ticker won't be incremented anymore to avoid having a negative time span.
		Draw(smoothratio);
		if (state == skipped) return -1;
		if (state == finished) return 0;
		return Frame(clock, skiprequest);
	}

	int GetFadeState() const { return fadestate; }

};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DSkippableScreenJob : public DScreenJob
{
protected:
	DSkippableScreenJob(int fade = 0, float fadet = 250.f) : DScreenJob(fade, fadet)
	{}

	bool OnEvent(event_t* evt) override;
	virtual void Skipped() {}
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DBlackScreen : public DScreenJob
{
	int wait;
	bool cleared = false;

public:
	DBlackScreen(int w) : wait(w) {}
	void OnTick() override;
	void Draw(double smooth) override;
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DImageScreen : public DSkippableScreenJob
{
	DECLARE_CLASS(DImageScreen, DScreenJob)

	int tilenum = -1;
	int trans;
	int waittime; // in ms.
	bool cleared = false;
	FGameTexture* tex = nullptr;

public:
	DImageScreen(FGameTexture* tile, int fade = DScreenJob::fadein | DScreenJob::fadeout, int wait = 3000, int translation = 0) : DSkippableScreenJob(fade), waittime(wait)
	{
		tex = tile;
		trans = translation;
	}

	DImageScreen(int tile, int fade = DScreenJob::fadein | DScreenJob::fadeout, int wait = 3000, int translation = 0) : DSkippableScreenJob(fade), waittime(wait)
	{
		tilenum = tile;
		trans = translation;
	}

	void OnTick() override;
	void Draw(double smooth) override;
};




struct JobDesc
{
	DScreenJob* job;
	void (*postAction)();
	bool ignoreifskipped;
};


void RunScreenJob(JobDesc *jobs, int count, CompletionFunc completion, bool clearbefore = true, bool blockingui = false);
void EndScreenJob();
void DeleteScreenJob();
bool ScreenJobResponder(event_t* ev);
void ScreenJobTick();
void ScreenJobDraw();

struct AnimSound
{
	int framenum;
	int soundnum;
};

DScreenJob *PlayVideo(const char *filename, const AnimSound *ans = nullptr, const int *frameticks = nullptr, bool nosoundstop = false);
