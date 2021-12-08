#include "build.h"
#include "hw_sections2.h"

int GetWindingOrder(TArray<int>& walls)
{
    int check = -1;
    int minY = INT_MAX;
    int minXAtMinY = INT_MAX;
    for (unsigned i = 0; i < walls.Size(); i++)
    {
        auto wal = &wall[walls[i]];
        int y = wal->y;

		if (y < minY)
		{
			minY = y;
			minXAtMinY = INT_MAX,	// must reset this if a new y is picked.
			check = i;
		}
		else if (y == minY && wal->x < minXAtMinY)
		{
			minXAtMinY = wal->x;
		}
    }
	
	unsigned prev = check == 0? walls.Size() - 1 : check - 1;
	unsigned next = check == walls.Size()? 0 : check + 1;
	
	DVector2 a = { (double)wall[walls[prev]].x, (double)wall[walls[prev]].y };
	DVector2 b = { (double)wall[walls[check]].x, (double)wall[walls[check]].y };
	DVector2 c = { (double)wall[walls[next]].x, (double)wall[walls[next]].y };

    return (b.X * c.Y + a.X * b.Y + a.Y * c.X) - (a.Y * b.X + b.Y * c.X + a.X * c.Y) > 0;
}

struct loopcollect
{
	TArray<TArray<int>> loops;
	bool bugged;
};

void CollectLoops(TArray<loopcollect>& sectors)
{
	BitArray visited;
	visited.Resize(numwalls);
	visited.Zero();

	TArray<int> thisloop;
	
	int count = 0;
	for (int i = 0; i < numsectors; i++)
	{
		int first = sector[i].wallptr;
		int last = first + sector[i].wallnum;
		sectors.Reserve(1);
		sectors.Last().bugged = false;
		
		for (int w = first; w < last; w++)
		{
			if (visited[w]) continue;
			thisloop.Clear();
			thisloop.Push(w);
			
			for (int ww = wall[w].point2; ww != w; ww = wall[ww].point2)
			{
				if (ww < first || ww >= last)
				{
					Printf("Found wall %d outside sector %d in a loop\n", ww, i);
					sectors.Last().bugged = true;
					break;
				}
				if (visited[ww])
				{
					Printf("Wall %d's point2 links to already visited wall %d\n", w, ww);
					sectors.Last().bugged = true;
					break;
				}
				thisloop.Push(ww);
				visited.Set(ww);
			}
			count ++;
			sectors.Last().loops.Push(std::move(thisloop));
		}
	}
	Printf("Created %d loops from %d sectors, %d walls\n", count, numsectors, numwalls);
}


void hw_CreateSections2()
{
	TArray<loopcollect> sectors;
	CollectLoops(sectors);
}
