// (weapons)
class BloodWeaponBase : BloodActor 
{
	meta int count;
	meta int type;
	meta int ammotype;
	
	property prefix: none;
	property count: count;
	property type: type;
	property ammotype: ammotype;
	
	override int getRespawnTime()
	{
		if (!self.hasX) return -1;
		if (self.xspr.respawn == 2 || (self.xspr.respawn != 1 && gGameOptions.nWeaponSettings != Blood.WEAPONSETTINGS_0))
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}
	
}

class BloodWeaponSawedoff : BloodWeaponBase
{
	default
	{
		pic "ICONSHOTGUN";
		shade -8;
		scale 0.750000, 0.750000;
		count 8;
		type 3;
		ammotype 2;
	}
}
class BloodWeaponTommygun : BloodWeaponBase
{
	default
	{
		pic "ICONTOMMY";
		shade -8;
		scale 0.750000, 0.750000;
		count 50;
		type 4;
		ammotype 3;
	}
}
class BloodWeaponFlarePistol : BloodWeaponBase
{
	default
	{
		pic "ICONFLAREGUN";
		shade -8;
		scale 0.750000, 0.750000;
		count 9;
		type 2;
		ammotype 1;
	}
}
class BloodWeaponVoodooDoll : BloodWeaponBase
{
	default
	{
		pic "AmmoIcon9";
		shade -8;
		scale 0.750000, 0.750000;
		count 100;
		type 10;
		ammotype 9;
	}
}
class BloodWeaponTeslaCannon : BloodWeaponBase
{
	default
	{
		pic "ICONTESLA";
		shade -8;
		scale 0.750000, 0.750000;
		count 64;
		type 8;
		ammotype 7;
	}
}
class BloodWeaponNapalmLauncher : BloodWeaponBase
{
	default
	{
		pic "ICONNAPALM";
		shade -8;
		scale 0.750000, 0.750000;
		count 6;
		type 5;
		ammotype 4;
	}
}
class BloodWeaponPitchfork : BloodWeaponBase
{
	default
	{
		type 1;
	}
}
class BloodWeaponSprayCan : BloodWeaponBase
{
	default
	{
		pic "AmmoIcon6";
		shade -8;
		scale 0.750000, 0.750000;
		count 480;
		type 7;
		ammotype 6;
	}
}
class BloodWeaponTNT : BloodWeaponBase
{
	default
	{
		pic "AmmoIcon5";
		shade -8;
		scale 0.750000, 0.750000;
		count 1;
		type 6;
		ammotype 5;
	}
}
class BloodWeaponLifeLeech : BloodWeaponBase
{
	default
	{
		pic "ICONLEECH";
		shade -8;
		scale 0.750000, 0.750000;
		count 35;
		type 9;
		ammotype 8;
	}
}

// items (ammos)
class BloodAmmoBase : BloodActor 
{
	meta int count;
	meta int type;
	meta int weapontype;
	
	property prefix: none;
	property count: count;
	property type: type;
	property weapontype: weapontype;
	
	override int getRespawnTime()
	{
		if (!self.hasX) return -1;
		if (self.xspr.respawn == 3 || gGameOptions.nWeaponSettings == Blood.WEAPONSETTINGS_1) return 0;
		else if (self.xspr.respawn != 1 && gGameOptions.nWeaponSettings != Blood.WEAPONSETTINGS_0)
			return gGameOptions.nWeaponRespawnTime;
		return -1;
	}
	
}

class BloodAmmoSprayCan : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon6";
		shade -8;
		scale 0.625, 0.625;
		count 480;
		type 6;
		weapontype 7;
	}
}
class BloodAmmoTNTBundle : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon5";
		shade -8;
		scale 0.75, 0.75;
		count 1;
		type 5;
		weapontype 6;
	}
}
class BloodAmmoTNTBox : BloodAmmoBase
{
	default
	{
		pic "AmmoTNTBox";
		shade -8;
		scale 0.75, 0.75;
		count 5;
		type 5;
		weapontype 6;
	}
}
class BloodAmmoProxBombBundle : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon10";
		shade -8;
		scale 0.75, 0.75;
		count 1;
		type 10;
		weapontype 11;
	}
}
class BloodAmmoRemoteBombBundle : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon11";
		shade -8;
		scale 0.75, 0.75;
		count 1;
		type 11;
		weapontype 12;
	}
}
class BloodAmmoTrappedSoul : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon8";
		shade -8;
		scale 0.375, 0.375;
		count 10;
		type 8;
		weapontype 0;
	}
}
class BloodAmmoSawedoffFew : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon2";
		shade -8;
		scale 0.75, 0.75;
		count 4;
		type 2;
		weapontype 0;
	}
}
class BloodAmmoSawedoffBox : BloodAmmoBase
{
	default
	{
		pic "AmmoShotgunFew";
		shade -8;
		scale 0.75, 0.75;
		count 15;
		type 2;
		weapontype 0;
	}
}
class BloodAmmoTommygunFew : BloodAmmoBase
{
	default
	{
		pic "AmmoShotgunBox";
		shade -8;
		scale 0.75, 0.75;
		count 15;
		type 3;
		weapontype 0;
	}
}
class BloodAmmoVoodooDoll : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon9";
		shade -8;
		scale 0.75, 0.75;
		count 100;
		type 9;
		weapontype 10;
	}
}
class BloodAmmoTommygunDrum : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon3";
		shade -8;
		scale 0.75, 0.75;
		count 100;
		type 3;
		weapontype 0;
	}
}
class BloodAmmoTeslaCharge : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon7";
		shade -8;
		scale 0.375, 0.375;
		count 32;
		type 7;
		weapontype 0;
	}
}
class BloodAmmoFlares : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon1";
		shade -8;
		scale 0.75, 0.75;
		count 8;
		type 1;
		weapontype 0;
	}
}
class BloodAmmoGasolineCan : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon4";
		shade -8;
		scale 0.75, 0.75;
		count 6;
		type 4;
		weapontype 0;
	}
}
// while these are marked obsolete we need to define them to work to the degree they originally do, i.e. they need their spawn info set.
class BloodAmmoObsolete61 : BloodAmmoBase
{
	default
	{
		pic "AmmoIcon5";
		shade -8;
		scale 0.75, 0.75;
		count 1;
		type 5;
		weapontype 6;
	}
}
class BloodAmmoObsolete71 : BloodAmmoBase
{
	default
	{
		pic "AmmoUseless1";
		shade -8;
		scale 0.75, 0.75;
		count 15;
		type 255;
		weapontype 0;
	}
}
class BloodAmmoObsolete74 : BloodAmmoBase
{
	default
	{
		shade -8;
		scale 0.75, 0.75;
		count 6;
		type 255;
		weapontype 0;
	}
}
class BloodAmmoObsolete75 : BloodAmmoBase
{
	default
	{
		shade -8;
		scale 0.75, 0.75;
		count 6;
		type 255;
		weapontype 0;
	}
}
class BloodAmmoObsolete77 : BloodAmmoBase
{
	default
	{
		pic "AmmoUseless2";
		shade -8;
		scale 0.75, 0.75;
		count 8;
		type 255;
		weapontype 0;
	}
}
class BloodAmmoObsolete78 : BloodAmmoBase
{
	default
	{
		pic "AmmoUseless3";
		shade -8;
		scale 0.75, 0.75;
		count 8;
		type 255;
		weapontype 0;
	}
}
