#pragma once


class StatIterator
{
    int next;
public:
    StatIterator(int stat)
    {
        assert(stat >= 0 && stat < MAXSTATUS);
        next = headspritestat[stat];
    }
    
    void Reset(int stat)
    {
        assert(stat >= 0 && stat < MAXSTATUS);
        next = headspritestat[stat];
    }
    
    int NextIndex()
    {
        int n = next;
        if (n >= 0) next = nextspritestat[next];
        return n;
    }

    int PeekIndex()
    {
        return next;
    }


    // These are only used by one particularly screwy loop in Blood's nnexts.cpp.
    static int First(int stat)
    {
        return headspritestat[stat];
    }

    static int NextFor(int spr)
    {
        return nextspritestat[spr];
    }
};

class SectIterator
{
    int next;
public:
    SectIterator(int stat)
    {
        assert(stat >= 0 && stat < MAXSECTORS);
        next = headspritesect[stat];
    }
	
	SectIterator(sectortype* sect)
	{
		assert(sect);
		next = headspritesect[sect - sector];
	}
    
    void Reset(int stat)
    {
        assert(stat >= 0 && stat < MAXSECTORS);
        next = headspritesect[stat];
    }
    
	void Reset(sectortype* sect)
	{
		assert(sect);
		next = headspritesect[sect - sector];
	}
	
    int NextIndex()
    {
        int n = next;
        if (n >= 0) next = nextspritesect[next];
        return n;
    }
	
	int PeekIndex()
	{
		return next;
	}
};
