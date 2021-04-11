//--------------------------------------- VOX LIBRARY BEGINS ---------------------------------------

#ifdef USE_OPENGL

#include "compat.h"
#include "build.h"
#include "engine_priv.h"
#include "polymost.h"
#include "mdsprite.h"
#include "v_video.h"
#include "flatvertices.h"
#include "hw_renderstate.h"
#include "texturemanager.h"
#include "voxels.h"
#include "gamecontrol.h"
#include "glbackend/gl_models.h"

#include "palette.h"
#include "../../glbackend/glbackend.h"

void voxfree(voxmodel_t *m)
{
    if (!m)
        return;

    Xfree(m);
}

voxmodel_t *voxload(int lumpnum)
{
    FVoxel* voxel = R_LoadKVX(lumpnum);
    if (voxel != nullptr)
    {
        voxmodel_t* vm = (voxmodel_t*)Xmalloc(sizeof(voxmodel_t));
        memset(vm, 0, sizeof(voxmodel_t));
        auto pivot = voxel->Mips[0].Pivot;
        vm->mdnum = 1; //VOXel model id
        vm->scale = vm->bscale = 1.f;
        vm->piv.x = float(pivot.X);
        vm->piv.y = float(pivot.Y);
        vm->piv.z = float(pivot.Z);
        vm->siz.x = voxel->Mips[0].SizeX;
        vm->siz.y = voxel->Mips[0].SizeY;
        vm->siz.z = voxel->Mips[0].SizeZ;
        vm->is8bit = true;
        voxel->Mips[0].Pivot.Zero();  // Needs to be taken out of the voxel data because it gets baked into the vertex buffer which we cannot use here.
        vm->model = new FVoxelModel(voxel, true);
        return vm;
    }
    return nullptr;
}

//Draw voxel model as perfect cubes
int32_t polymost_voxdraw(voxmodel_t* m, tspriteptr_t const tspr, bool rotate)
{
    float f, g, k0, zoff;

    if ((intptr_t)m == (intptr_t)(-1)) // hackhackhack
        return 0;

    if ((tspr->cstat & 48) == 32)
        return 0;

    polymost_outputGLDebugMessage(3, "polymost_voxdraw(m:%p, tspr:%p)", m, tspr);

    //updateanimation((md2model *)m,tspr);

    if ((tspr->cstat & CSTAT_SPRITE_MDLROTATE) || rotate)
    {
        int myclock = (PlayClock << 3) + MulScale(4 << 3, pm_smoothratio, 16);
        tspr->ang = (tspr->ang + myclock) & 2047; // will be applied in md3_vox_calcmat_common.
    }


    vec3f_t m0 = { m->scale, m->scale, m->scale };
    vec3f_t a0 = { 0, 0, m->zadd*m->scale };

    k0 = m->bscale / 64.f;
    f = (float) tspr->xrepeat * (256.f/320.f) * k0;
    if ((sprite[tspr->owner].cstat&48)==16)
    {
        f *= 1.25f;
        a0.y -= tspr->xoffset * bcosf(spriteext[tspr->owner].angoff, -20);
        a0.x += tspr->xoffset * bsinf(spriteext[tspr->owner].angoff, -20);
    }

    if (globalorientation&8) { m0.z = -m0.z; a0.z = -a0.z; } //y-flipping
    if (globalorientation&4) { m0.x = -m0.x; a0.x = -a0.x; a0.y = -a0.y; } //x-flipping

    m0.x *= f; a0.x *= f; f = -f;
    m0.y *= f; a0.y *= f;
    f = (float) tspr->yrepeat * k0;
    m0.z *= f; a0.z *= f;

    k0 = (float) (tspr->z+spriteext[tspr->owner].position_offset.z);
    f = ((globalorientation&8) && (sprite[tspr->owner].cstat&48)!=0) ? -4.f : 4.f;
    k0 -= (tspr->yoffset*tspr->yrepeat)*f*m->bscale;
    zoff = m->siz.z*.5f;
    if (!(tspr->cstat&128))
        zoff += m->piv.z;
    else if ((tspr->cstat&48) != 48)
    {
        zoff += m->piv.z;
        zoff -= m->siz.z*.5f;
    }
    if (globalorientation&8) zoff = m->siz.z-zoff;

    f = (65536.f*512.f) / ((float)xdimen*viewingrange);
    g = 32.f / ((float)xdimen*gxyaspect);

    int const shadowHack = !!(tspr->clipdist & TSPR_FLAGS_MDHACK);

    m0.y *= f; a0.y = (((float)(tspr->x+spriteext[tspr->owner].position_offset.x-globalposx)) * (1.f/1024.f) + a0.y) * f;
    m0.x *=-f; a0.x = (((float)(tspr->y+spriteext[tspr->owner].position_offset.y-globalposy)) * -(1.f/1024.f) + a0.x) * -f;
    m0.z *= g; a0.z = (((float)(k0     -globalposz - shadowHack)) * -(1.f/16384.f) + a0.z) * g;

    float mat[16];
    md3_vox_calcmat_common(tspr, &a0, f, mat);

    //Mirrors
    if (grhalfxdown10x < 0)
    {
        mat[0] = -mat[0];
        mat[4] = -mat[4];
        mat[8] = -mat[8];
        mat[12] = -mat[12];
    }

    if (shadowHack)
    {
		GLInterface.SetDepthFunc(DF_LEqual);
	}


	int winding = ((grhalfxdown10x >= 0) ^ ((globalorientation & 8) != 0) ^ ((globalorientation & 4) != 0)) ? Winding_CW : Winding_CCW;
	GLInterface.SetCull(Cull_Back, winding);

    float pc[4];

    if (!shadowHack)
    {
        pc[3] = (tspr->cstat & 2) ? glblend[tspr->blend].def[!!(tspr->cstat & 512)].alpha : 1.0f;
        pc[3] *= 1.0f - spriteext[tspr->owner].alpha;

        SetRenderStyleFromBlend(!!(tspr->cstat & 2), tspr->blend, !!(tspr->cstat & 512));

        if (!(tspr->cstat & 2) || spriteext[tspr->owner].alpha > 0.f || pc[3] < 1.0f)
            GLInterface.EnableBlend(true);  // else GLInterface.EnableBlend(false);
    }
    else pc[3] = 1.f;
	GLInterface.SetShade(std::max(0, globalshade), numshades);

    pc[0] = (float)globalr * (1.f / 255.f);
    pc[1] = (float)globalg * (1.f / 255.f);
    pc[2] = (float)globalb * (1.f / 255.f);
    GLInterface.SetColor(pc[0], pc[1], pc[2], pc[3]);

    //------------

    //transform to Build coords
    float omat[16];
    memcpy(omat, mat, sizeof(omat));

    f = 1.f/64.f;
    g = m0.x*f; mat[0] *= g; mat[1] *= g; mat[2] *= g;
    g = m0.y*f; mat[4] = omat[8]*g; mat[5] = omat[9]*g; mat[6] = omat[10]*g;
    g =-m0.z*f; mat[8] = omat[4]*g; mat[9] = omat[5]*g; mat[10] = omat[6]*g;
    //
    mat[12] -= (m->piv.x*mat[0] + m->piv.y*mat[4] + zoff*mat[8]);
    mat[13] -= (m->piv.x*mat[1] + m->piv.y*mat[5] + zoff*mat[9]);
    mat[14] -= (m->piv.x*mat[2] + m->piv.y*mat[6] + zoff*mat[10]);
    //
    //Let OpenGL (and perhaps hardware :) handle the matrix rotation
    mat[3] = mat[7] = mat[11] = 0.f; mat[15] = 1.f;

    for (int i = 0; i < 15; i++) mat[i] *= 1024.f;

    // Adjust to backend coordinate system being used by the vertex buffer.
    for (int i = 4; i < 8; i++)
    {
        float f = mat[i];
        mat[i] = -mat[i + 4];
        mat[i + 4] = -f;
    }

    GLInterface.SetMatrix(Matrix_Model, mat);

    int palId = TRANSLATION(Translation_Remap + curbasepal, globalpal);
    GLInterface.SetPalswap(globalpal);
    GLInterface.SetFade(sector[tspr->sectnum].floorpal);

    auto tex = TexMan.GetGameTexture(m->model->GetPaletteTexture());
    GLInterface.SetTexture(tex, TRANSLATION(Translation_Remap + curbasepal, globalpal), CLAMP_NOFILTER_XY, true);
    GLInterface.SetModel(m->model, 0, 0, 0);
    GLInterface.Draw(DT_Triangles, 0, 0);
    GLInterface.SetModel(nullptr, 0, 0, 0);
	GLInterface.SetCull(Cull_None);

    if (shadowHack)
    {
		GLInterface.SetDepthFunc(DF_Less);
	}
	GLInterface.SetIdentityMatrix(Matrix_Model);
	GLInterface.SetFadeDisable(false);
    return 1;
}
#endif

//---------------------------------------- VOX LIBRARY ENDS ----------------------------------------
