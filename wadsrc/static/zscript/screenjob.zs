
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
	virtual void OnSkip() {}

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

//---------------------------------------------------------------------------
//
// this is to have a unified interface to the summary screens
// that can be set up automatically by the games to avoid direct access to game data.
//
//---------------------------------------------------------------------------

class SummaryScreenBase : ScreenJob
{
	MapRecord level;
	int kills, maxkills;
	int secrets, maxsecrets, supersecrets;
	int playtime;
	bool cheatflag;

	void SetParameters(MapRecord map, int kills_, int maxkills_, int secrets_, int maxsecrets_, int supersecrets_, int time_, bool cheated)
	{
		level = map;
		kills = kills_;
		maxkills = maxkills_;
		secrets = secrets_;
		maxsecrets = maxsecrets_;
		supersecrets = supersecrets_;
		playtime = time_;
		cheatflag = cheated;
	}

	String FormatTime(int time)
	{
		return String.Format("%02d:%02d", (time / (26 * 60)) % 60, (time / 26) % 60);
	}

}
