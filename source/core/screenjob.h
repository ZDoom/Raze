#pragma once
#include <functional>
#include "dobject.h"
#include "v_2ddrawer.h"
#include "d_eventbase.h"
#include "s_soundinternal.h"

using CompletionFunc = std::function<void(bool)>;
struct JobDesc;
class ScreenJobRunner;

#if 0
class DScreenJob : public DObject
{
	DECLARE_CLASS(DScreenJob, DObject)
public:
	const int flags;
	const float fadetime;	// in milliseconds
	int fadestate = fadein;

	friend class ScreenJobRunner;
//protected:
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
		stopmusic = 4,
		stopsound = 8,
	};

	DScreenJob(int fade = 0, float fadet = 250.f) : flags(fade), fadetime(fadet) {}
	
	virtual bool ProcessInput()
	{
		return false;
	}

	virtual void Start() {}
	virtual bool OnEvent(event_t* evt) { return false; }
	virtual void OnTick() { /*state = finished;*/ }
	virtual void Draw(double smoothratio) {}

	int DrawFrame(double smoothratio)
	{
		if (state != running) smoothratio = 1; // this is necessary because the ticker won't be incremented anymore to avoid having a negative time span.
		Draw(smoothratio);
		if (state == skipped) return -1;
		if (state == finished) return 0;
		return 1;
	}

	int GetFadeState() const { return fadestate; }

	virtual void OnDestroy() override;
};

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class DSkippableScreenJob : public DScreenJob
{
	DECLARE_CLASS(DSkippableScreenJob, DScreenJob)
public:
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
	DECLARE_CLASS(DBlackScreen, DScreenJob)
public:
	int wait;
	bool cleared = false;

public:
	DBlackScreen(int w, int flags = 0) : DScreenJob(flags & ~(fadein|fadeout)), wait(w) {}
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

public:
	int tilenum = -1;
	int trans;
	int waittime; // in ms.
	bool cleared = false;
	FTextureID texid;
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
#endif

#if 0
//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class ScreenJobRunner
{
	enum
	{
		State_Clear,
		State_Run,
		State_Fadeout
	};
	TArray<DScreenJob*> jobs;
	CompletionFunc completion;
	int index = -1;
	float screenfade;
	bool clearbefore;
	bool skipall;
	int actionState;
	int terminateState;
	int fadeticks = 0;
	int last_paused_tic = -1;

public:
	ScreenJobRunner(TArray<DScreenJob*>& jobs, CompletionFunc completion_, bool clearbefore_, bool skipall_);
	~ScreenJobRunner();
	void DeleteJobs();
	void AdvanceJob(bool skip);
	int DisplayFrame(double smoothratio);
	int FadeoutFrame(double smoothratio);
	bool OnEvent(event_t* ev);
	void OnFinished();
	void OnTick();
	bool RunFrame();
};
#endif


#if 0
enum
{
	SJ_DONTCLEAR = 1,
	SJ_BLOCKUI = 2,
	SJ_SKIPALL = 4
};


void RunScreenJob(TArray<DScreenJob*>& jobs, CompletionFunc completion, int flags = 0);
#endif
void EndScreenJob();
void DeleteScreenJob();
bool ScreenJobResponder(event_t* ev);
void ScreenJobTick();
bool ScreenJobDraw();

#if 0
DScreenJob *PlayVideo(const char *filename, const AnimSound *ans = nullptr, const int *frameticks = nullptr, bool nosoundstop = false);
#endif