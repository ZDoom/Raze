#include "basics.h"
#include "model.h"
#include "modeldata.h"
#include "texturemanager.h"
#include "hw_voxels.h"

#include "build.h"


int ModelManager::LoadModel(const char* fn)
{
    unsigned modelid = FindModel(nullptr, fn, true);

    if (modelid == ~0u)
        return -1;

    // Note that the descriptor table may contain the same model multiple times!

    int mdid = modelDescs.Reserve(1);
    modelDescs.Last() = {};
    modelDescs.Last().modelID = modelid;
    return mdid;
}

int ModelManager::SetMisc(int modelid, float scale, int shadeoff, float zadd, float yoffset, int flags)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;

    auto md = &modelDescs[modelid];
    md->bscale = scale;
    md->shadeoff = shadeoff;
    md->zadd = zadd;
    md->yoffset = yoffset;
    md->flags = flags;
    return 0;
}

int ModelManager::DefineFrame(int modelid, const char* framename, int tilenum, int skinnum, float smoothduration, int pal)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;
    if ((uint32_t)tilenum >= MAXTILES) return -2;
    if (!framename) return -3;

    ModelTileFrame mframe;

    auto mdesc = &modelDescs[modelid];
    auto model = Models[mdesc->modelID];

    auto frm = model->FindFrame(framename, true);
    if (frm == FErr_NotFound) return -3;
    if (frm == FErr_Voxel) skinnum = 0;
    if (frm < 0) frm = 0;

    mframe.modelid = modelid;
    mframe.framenum = frm;
    mframe.skinnum = skinnum;
    mframe.smoothduration = smoothduration;
    auto key = FrameMapKey(tilenum, pal);
    frameMap.Insert(key, mframe);
    return key;
}

int ModelManager::DefineAnimation(int modelid, const char* framestart, const char* frameend, int fpssc, int flags)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;
    if (!framestart) return -2;
    if (!frameend) return -3;

    auto mdesc = &modelDescs[modelid];
    auto model = Models[mdesc->modelID];

    auto frm = model->FindFrame(framestart, true);
    if (frm == FErr_NotFound) return -2;
    if (frm < 0) frm = 0;

    auto frme = model->FindFrame(frameend, true);
    if (frme == FErr_NotFound) return -3;
    if (frme < 0) frme = 0;

    ModelAnimation anm;

    anm.startframe = frm;
    anm.endframe = frme;
    anm.fpssc = fpssc;
    anm.flags = flags;
    mdesc->anims.Push(anm);
    return 0;
}

int ModelManager::DefineSkin(int modelid, const char* skinfn, int palnum, int skinnum, int surfnum, float param, float specpower, float specfactor, int flags)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;
    if (!skinfn) return -2;
    if ((unsigned)palnum >= (unsigned)MAXPALOOKUPS) return -3;

    auto mdesc = &modelDescs[modelid];
    auto model = Models[mdesc->modelID];

    if (model->FindFrame("", true) == FErr_Voxel)
        return 0;

    if (!model->hasSurfaces) surfnum = 0;

    ModelSkinDef* skdef = nullptr;

    for (auto& sk : mdesc->skins)
    {
        if (sk.palette == (uint8_t)palnum && skinnum == sk.skinnum && surfnum == sk.surfnum)
        {
            skdef = &sk;
            break;
        }
    }
    if (skdef == nullptr)
    {
        mdesc->skins.Reserve(1);
        skdef = &mdesc->skins.Last();
    }

    skdef->palette = (uint8_t)palnum;
    skdef->flags = (uint8_t)flags;
    skdef->skinnum = skinnum;
    skdef->surfnum = surfnum;
    skdef->param = param;
    skdef->specpower = specpower;
    skdef->specfactor = specfactor;
    skdef->texture = TexMan.CheckForTexture(skinfn, ETextureType::Any);
    if (!skdef->texture.isValid()) return -2;
    return 0;
}

int ModelManager::DefineHud(int modelid, int tilex, FVector3 add, int angadd, int flags, int fov)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;
    if ((unsigned)tilex >= (unsigned)MAXTILES) return -1;

    auto mdesc = &modelDescs[modelid];
    auto model = Models[mdesc->modelID];
    
    // not implemented yet

    return 0;
}

int ModelManager::UndefineTile(int tile)
{
    if ((unsigned)tile >= (unsigned)MAXTILES) return -1;

    // delete all entries from the map that reference this tile
    for (int i = 0; i < MAXPALOOKUPS; i++)
    {
        frameMap.Remove(FrameMapKey(tile, i));
    }
    return 0;
}

int ModelManager::UndefineModel(int modelid)
{
    if ((unsigned)modelid >= modelDescs.Size()) return -1;

    auto mdesc = &modelDescs[modelid];
    mdesc->deleted = true;
    mdesc->anims.Reset();
    mdesc->skins.Reset();
    if (modelid == (int)modelDescs.Size() - 1)
        modelDescs.Pop();
    return 0;
}

#if 0 // kept as reminders for later. We still need the prioritization code in mdloadskin, the rest needs to be redone

FGameTexture* mdloadskin(idmodel_t* m, int32_t number, int32_t pal, int32_t surf, bool* exact)
{
    int32_t i;
    mdskinmap_t* sk, * skzero = NULL;

    if (m->mdnum == 2)
        surf = 0;

    if ((unsigned)pal >= (unsigned)MAXPALOOKUPS)
        return 0;

    i = -1;
    for (sk = m->skinmap; sk; sk = sk->next)
    {
        if (sk->palette == pal && sk->skinnum == number && sk->surfnum == surf)
        {
            if (exact) *exact = true;
            //Printf("Using exact match skin (pal=%d,skinnum=%d,surfnum=%d) %s\n",pal,number,surf,skinfile);
            return TexMan.GetGameTexture(sk->texture);
        }
        //If no match, give highest priority to number, then pal.. (Parkar's request, 02/27/2005)
        else if ((sk->palette == 0) && (sk->skinnum == number) && (sk->surfnum == surf) && (i < 5)) { i = 5; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum == 0) && (sk->surfnum == surf) && (i < 4)) { i = 4; skzero = sk; }
        else if ((sk->palette == 0) && (sk->skinnum == 0) && (sk->surfnum == surf) && (i < 3)) { i = 3; skzero = sk; }
        else if ((sk->palette == 0) && (sk->skinnum == number) && (i < 2)) { i = 2; skzero = sk; }
        else if ((sk->palette == pal) && (sk->skinnum == 0) && (i < 1)) { i = 1; skzero = sk; }
        else if ((sk->palette == 0) && (sk->skinnum == 0) && (i < 0)) { i = 0; skzero = sk; }
    }

    // Special palettes do not get replacements
    if (pal >= (MAXPALOOKUPS - RESERVEDPALS))
        return 0;

    if (skzero)
    {
        //Printf("Using def skin 0,0 as fallback, pal=%d\n", pal);
        if (exact) *exact = false;
        return TexMan.GetGameTexture(skzero->texture);
    }
    else
        return nullptr;
}


void updateModelInterpolation()
{
    // this never worked - only left as a reference to sprext stuff.
    omdtims = mdtims;
    mdtims = I_msTime();

    TSpriteIterator<DCoreActor> it;
    while (auto actor = it.Next())
    {
        if ((mdpause && actor->sprext.mdanimtims) || (actor->sprext.renderflags & SPREXT_NOMDANIM))
            actor->sprext.mdanimtims += mdtims - omdtims;
    }
}
#endif

//==========================================================================
//
// 
//
//==========================================================================

int tilehasmodelorvoxel(int const tilenume, int pal)
{
    return
        (hw_models && modelManager.CheckModel(tilenume, pal)) ||
        (r_voxels && tiletovox[tilenume] != -1);
}

