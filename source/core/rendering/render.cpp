// renderer draft. This code is not for release!
#include "glbackend/glbackend.h"
#include "build.h"
#include "hw_vrmodes.h"
#include "v_draw.h"
#include "gamecvars.h"
#include "binaryangle.h"
#include "automap.h"
#include "hw_clipper.h"
#include "hw_drawstructs.h"
#include "hw_clock.h"
#include "render.h"
#include "printf.h"
#include "v_video.h"
#include "flatvertices.h"
#include "gamefuncs.h"
#include "hw_drawinfo.h"

angle_t FrustumAngle(float ratio, float fov, float pitch)
{
    float tilt = fabs(pitch);

    // If the pitch is larger than this you can look all around at a FOV of 90°
    if (tilt > 46.0f) return 0xffffffff;

    // ok, this is a gross hack that barely works...
    // but at least it doesn't overestimate too much...
    // todo: integrate roll into the calculation
    double floatangle = 2.0 + (45.0 + ((tilt / 1.9))) * fov * 48.0 / AspectMultiplier(ratio) / 90.0;
    angle_t a1 = DAngle(floatangle).BAMs();
    if (a1 >= ANGLE_180) return 0xffffffff;
    return a1;
}


#define NS namespace Newrender { // auto-format blocking #define.
NS

struct FBunch
{
    int sectnum;
    int startline;
    int endline;
    angle_t startangle; // in pseudo angles for the clipper
    angle_t endangle;
};

// ----------------------------------------------------------------------------
//
// Bunches are groups of continuous lines
// This array stores the amount of points per bunch,
// the view angles for each point and the line index for the starting line
//
// ----------------------------------------------------------------------------

class BunchDrawer
{
public:
    Clipper &clipper;
    int LastBunch;
    int StartTime;
    TArray<FBunch> Bunches;
    TArray<int> CompareData;
    double viewx, viewy;
    FixedBitArray<MAXSECTORS> gotsector;

    //==========================================================================
    //
    //
    //
    //==========================================================================
public:
    BunchDrawer(Clipper& c, vec2_t& view) : clipper(c)
    {
        viewx = view.x * (1/ 16.f);
        viewy = view.y * -(1/ 16.f);
        StartScene();
        clipper.SetViewpoint(DVector2(viewx, viewy));
        for (int i = 0; i < numwalls; i++)
        {
            // Precalculate the clip angles to avoid doing this repeatedly during level traversal.
            // Reverse the orientation so that startangle and endangle are properly ordered.
            wall[i].clipangle = 0 - clipper.PointToPseudoAngle(wall[i].x * (1 / 16.f), wall[i].y * (-1 / 16.f));
        }
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================
private:
    void StartScene()
    {
        LastBunch = 0;
        StartTime = I_msTime();
        Bunches.Clear();
        CompareData.Clear();
        gotsector.Zero();
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    void StartBunch(int sectnum, int linenum, angle_t startan, angle_t endan)
    {
        FBunch* bunch = &Bunches[LastBunch = Bunches.Reserve(1)];

        bunch->sectnum = sectnum;
        bunch->startline = bunch->endline = linenum;
        bunch->startangle = startan;
        bunch->endangle = endan;
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    void AddLineToBunch(int line, int newan)
    {
        Bunches[LastBunch].endline++;
        Bunches[LastBunch].endangle = newan;
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    void DeleteBunch(int index)
    {
        Bunches[index] = Bunches.Last();
        Bunches.Pop();
    }

    bool CheckClip(walltype* wal)
    {
        auto pt2 = &wall[wal->point2];
        sectortype* backsector = &sector[wal->nextsector];
        sectortype* frontsector = &sector[wall[wal->nextwall].nextsector];
        float bs_floorheight1;
        float bs_floorheight2;
        float bs_ceilingheight1;
        float bs_ceilingheight2;
        float fs_floorheight1;
        float fs_floorheight2;
        float fs_ceilingheight1;
        float fs_ceilingheight2;

        // Mirrors and horizons always block the view
        //if (linedef->special==Line_Mirror || linedef->special==Line_Horizon) return true;

        PlanesAtPoint(frontsector, wal->x, wal->y, &fs_ceilingheight1, &fs_floorheight1);
        PlanesAtPoint(frontsector, pt2->x, pt2->y, &fs_ceilingheight2, &fs_floorheight2);

        PlanesAtPoint(backsector, wal->x, wal->y, &bs_ceilingheight1, &bs_floorheight1);
        PlanesAtPoint(backsector, pt2->x, pt2->y, &bs_ceilingheight2, &bs_floorheight2);

        // now check for closed sectors! No idea if we really need the sky checks. We'll see.
        if (bs_ceilingheight1 <= fs_floorheight1 && bs_ceilingheight2 <= fs_floorheight2)
        {
            // backsector's ceiling is below frontsector's floor.
            if (frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY) return false; 
            return true;
        }

        if (fs_ceilingheight1 <= bs_floorheight1 && fs_ceilingheight2 <= bs_floorheight2) 
        {
            // backsector's floor is above frontsector's ceiling
            if (frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY) return false;
            return true;
        }

        if (bs_ceilingheight1 <= bs_floorheight1 && bs_ceilingheight2 <= bs_floorheight2) 
        {
            // backsector is closed
            if (frontsector->ceilingstat & backsector->ceilingstat & CSTAT_SECTOR_SKY) return false;
            if (frontsector->floorstat & backsector->floorstat & CSTAT_SECTOR_SKY) return false;
            return true;
        }

        return false;
    }

    //==========================================================================
    //
    // ClipLine
    // Clips the given segment
    //
    //==========================================================================

    enum
    {
        CL_Skip = 0,
        CL_Draw = 1,
        CL_Pass = 2,
    };


    int ClipLine(int line)
    {
        angle_t startAngle, endAngle;
        auto wal = &wall[line];

        startAngle = wal->clipangle;
        endAngle = wall[wal->point2].clipangle;

        // Back side, i.e. backface culling	- read: endAngle >= startAngle!
        if (startAngle - endAngle < ANGLE_180)
        {
            return CL_Skip;
        }

        if (!clipper.SafeCheckRange(startAngle, endAngle))
        {
            return CL_Skip;
        }

        if (wal->nextwall == -1 || (wal->cstat & CSTAT_WALL_1WAY) || CheckClip(wal))
        {
            // one-sided
            clipper.SafeAddClipRange(startAngle, endAngle);
            return CL_Draw;
        }
        else
        {
            return CL_Draw | CL_Pass;
        }
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    void ProcessBunch(int bnch)
    {
        FBunch* bunch = &Bunches[bnch];

        ClipWall.Clock();
        for (int i = bunch->startline; i <= bunch->endline; i++)
        {
            int clipped = ClipLine(i);

            if (clipped & CL_Draw)
            {
                show2dwall.Set(i);

                //if (gl_render_walls)
                {
                    SetupWall.Clock();

                    HWWall hwwall;
                    //Printf("Rendering wall %d\n", i);
                    hwwall.Process(nullptr, &wall[i], &sector[bunch->sectnum], wall[i].nextsector<0? nullptr : &sector[wall[i].nextsector]);
                    rendered_lines++;

                    SetupWall.Unclock();
                }
            }

            if (clipped & CL_Pass)
            {
                ClipWall.Unclock();
                ProcessSector(wall[i].nextsector);
                ClipWall.Clock();
            }
        }
        ClipWall.Unclock();
    }

    //==========================================================================
    //
    // 
    //
    //==========================================================================

    int WallInFront(int wall1, int wall2)
    {
        double x1s = WallStartX(wall1);
        double y1s = WallStartY(wall1);
        double x1e = WallEndX(wall1);
        double y1e = WallEndY(wall1);
        double x2s = WallStartX(wall2);
        double y2s = WallStartY(wall2);
        double x2e = WallEndX(wall2);
        double y2e = WallEndY(wall2);

        double dx = x1e - x1s;
        double dy = y1e - y1s;

        double t1 = PointOnLineSide(x2s, y2s, x1s, y1s, dx, dy);
        double t2 = PointOnLineSide(x2e, y2e, x1s, y1s, dx, dy);
        if (t1 == 0)
        {
            if (t2 == 0) return(-1);
            t1 = t2;
        }
        if (t2 == 0) t2 = t1;

        if ((t1 * t2) >= 0)
        {
            t2 = PointOnLineSide(viewx, viewy, x1s, y1s, dx, dy);
            return((t2 * t1) < 0);
        }

        dx = x2e - x2s;
        dy = y2e - y2s;
        t1 = PointOnLineSide(x1s, y1s, x2s, y2s, dx, dy);
        t2 = PointOnLineSide(x1e, y1e, x2s, y2s, dx, dy);
        if (t1 == 0)
        {
            if (t2 == 0) return(-1);
            t1 = t2;
        }
        if (t2 == 0) t2 = t1;
        if ((t1 * t2) >= 0)
        {
            t2 = PointOnLineSide(viewx, viewy, x2s, y2s, dx, dy);
            return((t2 * t1) >= 0);
        }
        return(-2);
    }

    //==========================================================================
    //
    // This is a bit more complicated than it looks because angles can wrap
    // around so we can only compare angle differences.
    //
    // Rules:
    // 1. Any bunch can span at most 180°.
    // 2. 2 bunches can never overlap at both ends
    // 3. if there is an overlap one of the 2 starting points must be in the
    //    overlapping area.
    //
    //==========================================================================

    int BunchInFront(FBunch* b1, FBunch* b2)
    {
        angle_t anglecheck, endang;

        if (b2->startangle - b1->startangle < b1->endangle - b1->startangle)
        {
            // we have an overlap at b2->startangle
            anglecheck = b2->startangle - b1->startangle;

            // Find the wall in b1 that overlaps b2->startangle
            for (int i = b1->startline; i <= b1->endline; i++)
            {
                endang = wall[wall[i].point2].clipangle - b1->startangle;
                if (endang > anglecheck)
                {
                    // found a line
                    int ret = WallInFront(b2->startline, i);
                    return ret;
                }
            }
        }
        else if (b1->startangle - b2->startangle < b2->endangle - b2->startangle)
        {
            // we have an overlap at b1->startangle
            anglecheck = b1->startangle - b2->startangle;

            // Find the wall in b2 that overlaps b1->startangle
            for (int i = b2->startline; i <= b2->endline; i++)
            {
                endang = wall[wall[i].point2].clipangle - b2->startangle;
                if (endang > anglecheck)
                {
                    // found a line
                    int ret = WallInFront(i, b1->startline);
                    return ret;
                }
            }
        }
        // we have no overlap
        return -1;
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    int FindClosestBunch()
    {
        int closest = 0;              //Almost works, but not quite :(

        CompareData.Clear();
        for (unsigned i = 1; i < Bunches.Size(); i++)
        {
            switch (BunchInFront(&Bunches[i], &Bunches[closest]))
            {
            case 0:		// i is in front
                closest = i;
                continue;

            case 1:	// i is behind
                continue;

            default:		// can't determine
                CompareData.Push(i);	// mark for later comparison
                continue;
            }
        }

        // we need to do a second pass to see how the marked bunches relate to the currently closest one.
        for (unsigned i = 0; i < CompareData.Size(); i++)
        {
            switch (BunchInFront(&Bunches[CompareData[i]], &Bunches[closest]))
            {
            case 0:		// is in front
                closest = CompareData[i];
                CompareData[i] = CompareData.Last();
                CompareData.Pop();
                i = 0;	// we need to recheck everything that's still marked.
                continue;

            case 1:	// is behind
                CompareData[i] = CompareData.Last();
                CompareData.Pop();
                i--;
                continue;

            default:
                continue;

            }
        }
        return closest;
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

    void ProcessSector(int sectnum)
    {
        if (gotsector[sectnum]) return;
        gotsector.Set(sectnum);

        Bsp.Clock();

        auto sect = &sector[sectnum];
        bool inbunch;
        angle_t startangle;

        //if (sect->validcount == StartTime) return;
        //sect->validcount = StartTime;

#if 0//ndef BUILD_TEST
        DoSector(sectnum, false);
#endif

        //Todo: process subsectors
        inbunch = false;
        for (int i = 0; i < sect->wallnum; i++)
        {
            auto thiswall = &wall[sect->wallptr + i];

#ifdef _DEBUG
            // For displaying positions in debugger
            DVector2 start = { WallStartX(thiswall), WallStartY(thiswall) };
            DVector2 end = { WallStartX(thiswall->point2), WallStartY(thiswall->point2) };
#endif
            angle_t ang1 = thiswall->clipangle;
            angle_t ang2 = wall[thiswall->point2].clipangle;

            if (ang1 - ang2 < ANGLE_180)
            {
                // Backside
                inbunch = false;
            }
            else if (!clipper.SafeCheckRange(ang1, ang2))
            {
                // is it visible?
                inbunch = false;
            }
            else if (!inbunch || ang2 - startangle >= ANGLE_180)
            {
                // don't let a bunch span more than 180° to avoid problems.
                // This limitation ensures that the combined range of 2
                // bunches will always be less than 360° which simplifies
                // the distance comparison code because it prevents a 
                // situation where 2 bunches may overlap at both ends.

                startangle = ang1;
                StartBunch(sectnum, sect->wallptr + i, ang1, ang2);
                inbunch = true;
            }
            else
            {
                AddLineToBunch(sect->wallptr + i, ang2);
            }
            if (thiswall->point2 != sect->wallptr + i + 1) inbunch = false;
        }
        Bsp.Unclock();
    }

    //==========================================================================
    //
    //
    //
    //==========================================================================

public:
    void RenderScene(int viewsector)
    {
        ProcessSector(viewsector);
        while (Bunches.Size() > 0)
        {
            int closest = FindClosestBunch();
            ProcessBunch(closest);
            DeleteBunch(closest);
        }
    }
};


//-----------------------------------------------------------------------------
//
// R_FrustumAngle
//
//-----------------------------------------------------------------------------

static void SetProjection(const FRotator& rotation, FAngle fov)
{
    auto vrmode = VRMode::GetVRMode(false);
    const int eyeCount = vrmode->mEyeCount;
    const auto& eye = vrmode->mEyes[0];

    int width = (windowxy2.x - windowxy1.x + 1);
    int height = (windowxy2.y - windowxy1.y + 1);
    float ratio = ActiveRatio(width, height, nullptr);
    float fovratio;

    if (ratio >= 1.3f)
    {
        fovratio = 1.333333f;
    }
    else
    {
        fovratio = ratio;
    }
    auto rotmat = eye.GetProjection(fov.Degrees, ratio, fovratio);
    renderSetProjectionMatrix(rotmat.get());
}

static void SetViewMatrix(const FRotator& angles, float vx, float vy, float vz, bool mirror, bool planemirror)
{
    float mult = mirror ? -1.f : 1.f;
    float planemult = planemirror ? -1.f : 1.f;// Level->info->pixelstretch : Level->info->pixelstretch;
    VSMatrix mViewMatrix;

    mViewMatrix.loadIdentity();
    mViewMatrix.rotate(angles.Roll.Degrees, 0.0f, 0.0f, 1.0f);
    mViewMatrix.rotate(angles.Pitch.Degrees, 1.0f, 0.0f, 0.0f);
    mViewMatrix.rotate(angles.Yaw.Degrees, 0.0f, mult, 0.0f);
    mViewMatrix.translate(vx * mult, -vz * planemult, -vy);
    mViewMatrix.scale(-mult, planemult, 1);
    renderSetViewMatrix(mViewMatrix.get());


}




}

using namespace Newrender;

void render_drawrooms_(vec3_t& position, int sectnum, fixed_t q16angle, fixed_t q16horizon, float rollang, float fov, bool mirror, bool planemirror)
{
    GLInterface.ClearDepth();
    GLInterface.EnableBlend(false);
    GLInterface.EnableAlphaTest(false);
    GLInterface.EnableDepthTest(true);
    GLInterface.SetDepthFunc(DF_LEqual);
    GLInterface.SetRenderStyle(LegacyRenderStyles[STYLE_Translucent]);


    FRotator rotation;
    rotation.Yaw = -90.f + q16ang(q16angle).asdeg();
    rotation.Pitch = -HorizToPitch(q16horizon);
    rotation.Roll = rollang;
    GLInterface.SetViewport(windowxy1.x, windowxy1.y, windowxy2.x - windowxy1.x + 1, windowxy2.y - windowxy1.y + 1);
    SetProjection(rotation, fov);
    SetViewMatrix(rotation, position.x / 16.f, -position.y / 16.f, -position.z / 256.f, mirror, planemirror);

    renderSetViewpoint(position.x / 16.f, -position.y / 16.f, -position.z / 256.f);
    renderSetVisibility((2 / 65536.f) * g_visibility / r_ambientlight); // (2 / 65536.f) is a magic factor to produce the same brightness as Polymost.
    renderBeginScene();

    Clipper clipper;
    // fixme: This does not consider the roll angle yet. Pitch disabled to get consistent values during testing.
    auto fa = FrustumAngle(16.f / 9, r_fov, 0);// rotation.Pitch.Degrees);

    angle_t rotang = q16ang(q16angle).asbam();
    clipper.SafeAddClipRangeRealAngles(rotang + fa, rotang - fa);

    Newrender::BunchDrawer drawer(clipper, position.vec2);

    drawer.RenderScene(sectnum);

    renderFinishScene();

    GLInterface.SetDepthFunc(DF_LEqual);
}



