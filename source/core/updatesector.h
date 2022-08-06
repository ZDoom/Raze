//-------------------------------------------------------------------------
/*
Copyright (C) 2022 Christoph Oelckers

This is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
//------------------------------------------------------------------------- 
//
// updatesector utilities. Uses a breadth-first algorithm similar 
// but not identical to EDuke32's updatesectorneighbor.


// checker functions for updatesector's template parameter.
inline int inside0(double x, double y, double z, const sectortype* sect)
{
    return inside(x, y, sect);
}

inline int insideZ(double x, double y, double z, const sectortype* sect)
{
    double cz, fz;
    getzsofslopeptr(sect, x, y, &cz, &fz);
    return (z >= cz && z <= fz && inside(x, y, sect) != 0);
}

template<class Inside>
void DoUpdateSector(double x, double y, double z, int* sectnum, double maxDistance, Inside checker)
{
    double maxDistSq = maxDistance * maxDistance;

    if (validSectorIndex(*sectnum))
    {
        if (checker(x, y, z, &sector[*sectnum]))
            return;

        BFSSearch search(sector.Size(), *sectnum);

        int iter = 0;
        for (unsigned listsectnum; (listsectnum = search.GetNext()) != BFSSearch::EOL;)
        {
            auto lsect = &sector[listsectnum];
            if (checker(x, y, z, lsect))
            {
                *sectnum = listsectnum;
                return;
            }

            for (auto& wal : wallsofsector(lsect))
            {
                if (wal.nextsector >= 0 && !search.Check(wal.nextsector) && (iter == 0 || SquareDistToSector(x, y, wal.nextSector()) <= maxDistSq))
                    search.Add(wal.nextsector);
            }
            iter++;
        }
    }
    *sectnum = -1;
}

template<class Inside>
int FindSector(double x, double y, double z, Inside checker)
{
    for (int i = (int)sector.Size() - 1; i >= 0; i--)
        if (checker(x, y, z, &sector[i]))
        {
            return i;
        }
    return -1;
}


constexpr int MAXUPDATESECTORDIST = 1536;

inline void updatesector(int x_, int y_, int* sectnum)
{
    double x = x_ * inttoworld;
    double y = y_ * inttoworld;

    DoUpdateSector(x, y, 0, sectnum, MAXUPDATESECTORDIST * inttoworld, inside0);
    if (*sectnum == -1) *sectnum = FindSector(x, y, 0, inside0);
}

inline void updatesectorz(int x_, int y_, int z_, int* sectnum)
{
    double x = x_ * inttoworld;
    double y = y_ * inttoworld;
    double z = z_ * zinttoworld;

    DoUpdateSector(x, y, z, sectnum, MAXUPDATESECTORDIST * inttoworld, insideZ);
    if (*sectnum == -1) *sectnum = FindSector(x, y, z, insideZ);
}

inline void updatesector(int const x, int const y, sectortype** const sectp)
{
	int sectno = *sectp? sector.IndexOf(*sectp) : -1;
	updatesector(x, y, &sectno);
	*sectp = sectno == -1? nullptr : &sector[sectno];
}

inline void updatesectorz(int x, int y, int z, sectortype** const sectp)
{
    int sectno = *sectp ? sector.IndexOf(*sectp) : -1;
    updatesectorz(x, y, z, &sectno);
    *sectp = sectno == -1 ? nullptr : &sector[sectno];
}

inline void updatesectorneighbor(int x, int y, sectortype** const sect, int maxDistance = MAXUPDATESECTORDIST)
{
	int sectno = *sect? sector.IndexOf(*sect) : -1;
    DoUpdateSector(x * inttoworld, y * inttoworld, 0, &sectno, maxDistance * inttoworld, inside0);
	*sect = sectno < 0? nullptr : &sector[sectno];
}
