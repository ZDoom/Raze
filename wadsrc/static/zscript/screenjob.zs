
class ScreenJob native
{
	native int flags;
	native float fadetime;	// in milliseconds
	native int fadestate;

	native int ticks;
	native int jobstate;
	native bool pausable;

	enum EJobState
	{
		running = 1,	// normal operation
		skipped = 2,	// finished by user skipping
		finished = 3,	// finished by completing its sequence
		stopping = 4,	// running ending animations / fadeout, etc. Will not accept more input.
		stopped = 5,	// we're done here.
	};
	enum EJobFlags
	{
		visible = 0,
		fadein = 1,
		fadeout = 2,
		stopmusic = 4,
		stopsound = 8,
	};

	native void Init(int flags = 0, float fadet = 250.f);
	native virtual bool ProcessInput();
	native virtual void Start();
	native virtual bool OnEvent(InputEvent evt);
	native virtual void OnTick();
	native virtual void Draw(double smoothratio);

	//native int DrawFrame(double smoothratio);
	//native int GetFadeState();
	//native override void OnDestroy();
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class SkippableScreenJob : ScreenJob native
{
	native void Init(int flags = 0, float fadet = 250.f);
	//native override bool OnEvent(InputEvent evt);
	virtual void Skipped() {}
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class BlackScreen : ScreenJob native
{
	native int wait;
	native bool cleared;

	native void Init(int w, int flags = 0);
	//override void OnTick();
	//override void Draw(double smooth);
}

//---------------------------------------------------------------------------
//
//
//
//---------------------------------------------------------------------------

class ImageScreen : SkippableScreenJob native
{
	native int tilenum;
	native int trans;
	native int waittime; // in ms.
	native bool cleared;
	native TextureID texid;

	native void Init(String tex, int fade = fadein | fadeout, int wait = 3000, int translation = 0);
	//override void OnTick();
	//override void Draw(double smooth);
}
