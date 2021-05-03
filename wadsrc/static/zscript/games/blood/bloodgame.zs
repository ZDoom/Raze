// contains all global Blood definitions
struct Blood native
{
	native static void PlayIntroMusic();
	native static bool OriginalLoadScreen(); // doing it generically would necessitate exporting the tile manage which we do not want.
	native static void sndStartSample(int resid, int volume, int channel, bool loop = false, int chanflags = 0);
	native static void sndStartSampleNamed(String sname, int volume, int channel);
}
