class DukeNewBeast : DukeActor
{
	const NEWBEASTSTRENGTH = 300;
	const NEWBEASTSCRATCHAMOUNT = -22;
	
	default
	{
		pic "NEWBEAST";
		Strength NEWBEASTSTRENGTH;
		+BADGUY;
		+KILLCOUNT;
		+GREENSLIMEFOOD;
		+GREENBLOOD;
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastStayput : DukeNewBeast
{
	default
	{
		pic "NEWBEASTSTAYPUT";
		+BADGUYSTAYPUT;
	}
	

}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastHang : DukeNewBeast
{
	default
	{
		pic "NEWBEASTHANG";
	}
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastHangDead : DukeNewBeast // (4671)
{
	default
	{
		pic "NEWBEASTHANGDEAD";
		-KILLCOUNT;
		Strength TOUGH;
	}
	
}

//---------------------------------------------------------------------------
//
// 
//
//---------------------------------------------------------------------------

class DukeNewBeastJump : DukeNewBeast // (4690)
{
	default
	{
		pic "NEWBEASTJUMP";
	}
	
	
}

