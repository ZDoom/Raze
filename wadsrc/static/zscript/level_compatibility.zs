
class LevelCompatibility : LevelPostProcessor
{
	protected void Apply(Name checksum, String mapname)
	{
		switch (checksum)
		{			
			case 'none':
				return;
		}
	}
}
