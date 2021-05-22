//---------------------------------------------------------------------------
//
// this is to have a unified interface to the summary screens
// that can be set up automatically by the games to avoid direct access to game data.
//
//---------------------------------------------------------------------------

class SummaryScreenBase : ScreenJob
{
	MapRecord level;
	SummaryInfo stats;

	void SetParameters(MapRecord map, SummaryInfo thestats)
	{
		level = map;
		stats = thestats;
	}

	String FormatTime(int time)
	{
		if (time >= 60 * 50)
			return String.Format("%02d:%02d:%02d", time / (60*60), (time / 60) % 60, time % 60);
		else
			return String.Format("%02d:%02d", (time / 60) % 60, time % 60);
	}

}

