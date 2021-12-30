
// Base class for game screens.
class GameScreen : Object ui
{
	virtual bool OnEvent(InputEvent ev)
	{
		return false;
	}

	virtual bool Tick(int framenum)
	{
		return false;
	}

	virtual void Draw()
	{
	}
}
