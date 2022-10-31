
class LevelPostProcessor native play
{
	protected void Apply(Name checksum, String mapname)
	{
	}

	protected native void SetSpriteLotag(int sprite, int tag);
	protected native void ChangeSpriteFlags(int sprite, int set, int clear);
	protected native void sw_serp_continue();
	protected native void SplitSector(int sect, int wal1, int wal2);
}
