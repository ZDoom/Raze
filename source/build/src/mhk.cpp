
#include "compat.h"
#include "build.h"
#include "baselayer.h"
#include "scriptfile.h"

usermaphack_t g_loadedMapHack;  // used only for the MD4 part

int compare_usermaphacks(const void *a, const void *b)
{
    return Bmemcmp(((usermaphack_t const *) a)->md4, ((usermaphack_t const *) b)->md4, 16);
}
usermaphack_t *usermaphacks;
int32_t num_usermaphacks;

#ifdef POLYMER
static int16_t maphacklightcnt=0;
static int16_t maphacklight[PR_MAXLIGHTS];

void engineClearLightsFromMHK()
{
    int32_t i;
    for (i=0; i<maphacklightcnt; i++)
    {
        if (maphacklight[i] >= 0)
            polymer_deletelight(maphacklight[i]);
        maphacklight[i] = -1;
    }

    maphacklightcnt = 0;
}
#else
void engineClearLightsFromMHK() {}
#endif

//
// loadmaphack
//
int32_t engineLoadMHK(const char *filename)
{
    enum
    {
        T_SPRITE = 0,
        T_ANGOFF,
        T_NOMODEL,
        T_NOANIM,
        T_PITCH,
        T_ROLL,
        T_MDXOFF,
        T_MDYOFF,
        T_MDZOFF,
        T_AWAY1,
        T_AWAY2,
        T_LIGHT,
    };

    static struct { const char *text; int32_t tokenid; } legaltokens [] =
    {
        { "sprite", T_SPRITE },
        { "angleoff", T_ANGOFF },
        { "angoff", T_ANGOFF },
        { "notmd2", T_NOMODEL },
        { "notmd3", T_NOMODEL },
        { "notmd", T_NOMODEL },
        { "nomd2anim", T_NOANIM },
        { "nomd3anim", T_NOANIM },
        { "nomdanim", T_NOANIM },
        { "pitch", T_PITCH },
        { "roll", T_ROLL },
        { "mdxoff", T_MDXOFF },
        { "mdyoff", T_MDYOFF },
        { "mdzoff", T_MDZOFF },
        { "away1", T_AWAY1 },
        { "away2", T_AWAY2 },
        { "light", T_LIGHT },
        { NULL, -1 }
    };

    scriptfile *script = NULL;
    char *tok, *cmdtokptr;
    int32_t i;
    int32_t whichsprite = -1;
    static char fn[BMAX_PATH];

#ifdef POLYMER
    int32_t toomanylights = 0;

    engineClearLightsFromMHK();
#endif

    if (filename)
    {
        Bmemset(spriteext, 0, sizeof(spriteext_t) * MAXSPRITES);
        Bmemset(spritesmooth, 0, sizeof(spritesmooth_t) *(MAXSPRITES+MAXUNIQHUDID));
        Bstrcpy(fn, filename);
        script = scriptfile_fromfile(filename);
    }
    else if (fn[0])
    {
        // re-load
        // XXX: what if we changed between levels? Could a wrong maphack be loaded?
        script = scriptfile_fromfile(fn);
    }

    if (!script)
    {
        fn[0] = 0;
        return -1;
    }

    while (1)
    {
        tok = scriptfile_gettoken(script);
        if (!tok) break;
        for (i=0; legaltokens[i].text; i++) if (!Bstrcasecmp(tok, legaltokens[i].text)) break;
        cmdtokptr = script->ltextptr;

        if (!filename && legaltokens[i].tokenid != T_LIGHT) continue;

        switch (legaltokens[i].tokenid)
        {
        case T_SPRITE:     // sprite <xx>
            if (scriptfile_getnumber(script, &whichsprite)) break;

            if ((unsigned) whichsprite >= (unsigned) MAXSPRITES)
            {
                // sprite number out of range
                initprintf("Sprite number out of range 0-%d on line %s:%d\n",
                    MAXSPRITES-1, script->filename, scriptfile_getlinum(script, cmdtokptr));
                whichsprite = -1;
                break;
            }

            break;
        case T_ANGOFF:     // angoff <xx>
        {
            int32_t ang;
            if (scriptfile_getnumber(script, &ang)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring angle offset directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].angoff = (int16_t) ang;
        }
        break;
        case T_NOMODEL:      // notmd
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring not-MD2/MD3 directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_NOTMD;
            break;
        case T_NOANIM:      // nomdanim
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring no-MD2/MD3-anim directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_NOMDANIM;
            break;
        case T_PITCH:     // pitch <xx>
        {
            int32_t pitch;
            if (scriptfile_getnumber(script, &pitch)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring pitch directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].pitch = (int16_t) pitch;
        }
        break;
        case T_ROLL:     // roll <xx>
        {
            int32_t roll;
            if (scriptfile_getnumber(script, &roll)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring roll directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].roll = (int16_t) roll;
        }
        break;
        case T_MDXOFF:     // mdxoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdxoff directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].offset.x = i;
        }
        break;
        case T_MDYOFF:     // mdyoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdyoff directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].offset.y = i;
        }
        break;
        case T_MDZOFF:     // mdzoff <xx>
        {
            int32_t i;
            if (scriptfile_getnumber(script, &i)) break;

            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring mdzoff directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].offset.z = i;
        }
        break;
        case T_AWAY1:      // away1
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring moving away directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_AWAY1;
            break;
        case T_AWAY2:      // away2
            if (whichsprite < 0)
            {
                // no sprite directive preceeding
                initprintf("Ignoring moving away directive because of absent/invalid sprite number on line %s:%d\n",
                    script->filename, scriptfile_getlinum(script, cmdtokptr));
                break;
            }
            spriteext[whichsprite].flags |= SPREXT_AWAY2;
            break;
#ifdef POLYMER
        case T_LIGHT:      // light sector x y z range r g b radius faderadius angle horiz minshade maxshade priority tilenum
        {
            int32_t value;
            int16_t lightid;
#pragma pack(push,1)
            _prlight light;
#pragma pack(pop)
            if (toomanylights)
                break;  // ignore further light defs

            scriptfile_getnumber(script, &value);
            light.sector = value;
            scriptfile_getnumber(script, &value);
            light.x = value;
            scriptfile_getnumber(script, &value);
            light.y = value;
            scriptfile_getnumber(script, &value);
            light.z = value;
            scriptfile_getnumber(script, &value);
            light.range = value;
            scriptfile_getnumber(script, &value);
            light.color[0] = value;
            scriptfile_getnumber(script, &value);
            light.color[1] = value;
            scriptfile_getnumber(script, &value);
            light.color[2] = value;
            scriptfile_getnumber(script, &value);
            light.radius = value;
            scriptfile_getnumber(script, &value);
            light.faderadius = value;
            scriptfile_getnumber(script, &value);
            light.angle = value;
            scriptfile_getnumber(script, &value);
            light.horiz = value;
            scriptfile_getnumber(script, &value);
            light.minshade = value;
            scriptfile_getnumber(script, &value);
            light.maxshade = value;
            scriptfile_getnumber(script, &value);
            light.priority = value;
            scriptfile_getsymbol(script, &value);
            light.tilenum = value;

            light.publicflags.emitshadow = 1;
            light.publicflags.negative = 0;

            if (videoGetRenderMode() == REND_POLYMER)
            {
                if (maphacklightcnt == PR_MAXLIGHTS)
                {
                    initprintf("warning: max light count %d exceeded, "
                        "ignoring further light defs\n", PR_MAXLIGHTS);
                    toomanylights = 1;
                    break;
                }

                lightid = polymer_addlight(&light);
                if (lightid>=0)
                    maphacklight[maphacklightcnt++] = lightid;
            }

            break;
        }
#endif // POLYMER

        default:
            // unrecognised token
            break;
        }
    }

    scriptfile_close(script);
    return 0;
}
