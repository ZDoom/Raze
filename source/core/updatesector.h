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
// updatesector utilities. Reimplementations of EDuke32's checks with
// proper C++ classes. (Original Build updatesector is insufficient and broken)


// checker functions for updatesector's template parameter.
inline int inside0(double x, double y, double z, const sectortype* sect)
{
    return inside(x, y, sect);
}

inline int insideZ(double x, double y, double z, const sectortype* sect)
{
    double cz, fz;
    calcSlope(sect, x, y, &cz, &fz);
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
        for (unsigned secnum; (secnum = search.GetNext()) != BFSSearch::EOL;)
        {
            auto lsect = &sector[secnum];
            if (checker(x, y, z, lsect))
            {
                *sectnum = secnum;
                return;
            }

            for (auto& wal : lsect->walls)
            {
                if (wal.twoSided() && !search.Check(wal.nextsector) && (iter == 0 || SquareDistToWall(x, y, &wal) <= maxDistSq))
                    search.Add(wal.nextsector);
            }
            iter++;
        }
    }

    for (int i = (int)sector.Size() - 1; i >= 0; i--)
        if (checker(x, y, z, &sector[i]))
        {
            *sectnum = i;
            return;
        }
    *sectnum = -1;
}


constexpr int MAXUPDATESECTORDIST = 96;

inline void updatesector(const DVector3& pos, sectortype** const sectp, double maxDistance = MAXUPDATESECTORDIST)
{
    int sectno = *sectp ? sector.IndexOf(*sectp) : -1;
	DoUpdateSector(pos.X, pos.Y, pos.Z, &sectno, maxDistance, inside0);
    *sectp = sectno == -1 ? nullptr : &sector[sectno];
}

inline void updatesector(const DVector2& pos, sectortype** const sectp, double maxDistance = MAXUPDATESECTORDIST)
{
	int sectno = *sectp ? sector.IndexOf(*sectp) : -1;
	DoUpdateSector(pos.X, pos.Y, 0, &sectno, maxDistance, inside0);
    *sectp = sectno == -1 ? nullptr : &sector[sectno];
}


inline void updatesectorz(const DVector3& pos, sectortype** const sectp, double maxDistance = MAXUPDATESECTORDIST)
{
	int sectno = *sectp ? sector.IndexOf(*sectp) : -1;
	DoUpdateSector(pos.X, pos.Y, pos.Z, &sectno, maxDistance, insideZ);
    *sectp = sectno == -1 ? nullptr : &sector[sectno];
}

