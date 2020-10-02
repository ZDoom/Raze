#pragma once

struct Item {

	using Callback = void (*)(PLAYER &plr, short i);
	
	int sizx, sizy;
	boolean treasures, cflag;
	Callback pickup;
	
	void Init(int sizx, int sizy, boolean treasure, boolean cflag, Callback call)
	{
		this->sizx = sizx;
		this->sizy = sizy;
		this->treasures = treasure;
		this->cflag = cflag;
		this->callback = call;
	}
};

