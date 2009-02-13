// blah
#ifdef POLYMOST

#define POLYMER_C
#include "polymer.h"

// CVARS
int32_t         pr_occlusionculling = 50;
int32_t         pr_fov = 426;           // appears to be the classic setting.
int32_t         pr_billboardingmode = 1;
int32_t         pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int32_t         pr_wireframe = 0;
int32_t         pr_vbos = 2;
int32_t         pr_mirrordepth = 1;
int32_t         pr_gpusmoothing = 1;

int32_t         glerror;

GLenum          mapvbousage = GL_STREAM_DRAW_ARB;
GLenum          modelvbousage = GL_STATIC_DRAW_ARB;

GLuint          modelvp;

// BUILD DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];

_prplane        spriteplane;
_prmaterial     mdspritematerial;

GLfloat         vertsprite[4 * 5] =
{
    -0.5f, 0.0f, 0.0f,
    0.0f, 1.0f,
    0.5f, 0.0f, 0.0f,
    1.0f, 1.0f,
    0.5f, 1.0f, 0.0f,
    1.0f, 0.0f,
    -0.5f, 1.0f, 0.0f,
    0.0f, 0.0f,
};

GLfloat         horizsprite[4 * 5] =
{
    -0.5f, 0.0f, -0.5f,
    0.0f, 1.0f,
    0.5f, 0.0f, -0.5f,
    1.0f, 1.0f,
    0.5f, 0.0f, 0.5f,
    1.0f, 0.0f,
    -0.5f, 0.0f, 0.5f,
    0.0f, 0.0f,
};

GLfloat         skyboxdata[4 * 5 * 6] =
{
    // -ZY
    -0.5f, -0.5f, 0.5f,
    0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,
    1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f,
    1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,
    0.0f, 0.0f,

    // XY
    -0.5f, -0.5f, -0.5f,
    0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,
    1.0f, 1.0f,
    0.5f, 0.5f, -0.5f,
    1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f,
    0.0f, 0.0f,

    // ZY
    0.5f, -0.5f, -0.5f,
    0.0f, 1.0f,
    0.5f, -0.5f, 0.5f,
    1.0f, 1.0f,
    0.5f, 0.5f, 0.5f,
    1.0f, 0.0f,
    0.5f, 0.5f, -0.5f,
    0.0f, 0.0f,

    // -XY
    0.5f, -0.5f, 0.5f,
    0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,
    1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f,
    1.0f, 0.0f,
    0.5f, 0.5f, 0.5f,
    0.0f, 0.0f,

    // XZ
    -0.5f, 0.5f, -0.5f,
    1.0f, 1.0f,
    0.5f, 0.5f, -0.5f,
    1.0f, 0.0f,
    0.5f, 0.5f, 0.5f,
    0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f,
    0.0f, 1.0f,

    // X-Z
    -0.5f, -0.5f, 0.5f,
    0.0f, 0.0f,
    0.5f, -0.5f, 0.5f,
    0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,
    1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,
    1.0f, 0.0f,
};

GLuint          skyboxdatavbo;

GLfloat         artskydata[16];

// LIGHTS
_prlight        prlights[PR_MAXLIGHTS];
int32_t         lightcount;

// MATERIALS
_prprogrambit   prprogrambits[PR_BIT_COUNT] = {
    {
        1 << PR_BIT_HEADER,
        // vert_def
        "#version 120\n"
        "\n",
        // vert_prog
        "",
        // frag_def
        "#version 120\n"
        "\n",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_NV4X_COMPAT,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "#define LIGHTCOUNT 4\n"
        "\n",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_G8X_COMPAT,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "#define LIGHTCOUNT lightCount\n"
        "\n",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_ANIM_INTERPOLATION,
        // vert_def
        "attribute vec4 nextFrameData;\n"
        "uniform float frameProgress;\n"
        "\n",
        // vert_prog
        "  vec4 currentFramePosition;\n"
        "  vec4 nextFramePosition;\n"
        "\n"
        "  currentFramePosition = gl_Vertex * (1.0 - frameProgress);\n"
        "  nextFramePosition = nextFrameData * frameProgress;\n"
        "  currentFramePosition = currentFramePosition + nextFramePosition;\n"
        "  result = gl_ModelViewProjectionMatrix * currentFramePosition;\n"
        "\n",
        // frag_def
        "",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_DIFFUSE_MAP,
        // vert_def
        "uniform vec2 diffuseScale;\n"
        "\n",
        // vert_prog
        "  gl_TexCoord[0] = vec4(diffuseScale, 1.0, 1.0) * gl_MultiTexCoord0;\n"
        "\n",
        // frag_def
        "uniform sampler2D diffuseMap;\n"
        "\n",
        // frag_prog
        "  result *= texture2D(diffuseMap, gl_TexCoord[0].st);\n"
        "\n",
    },
    {
        1 << PR_BIT_DIFFUSE_DETAIL_MAP,
        // vert_def
        "uniform vec2 detailScale;\n"
        "\n",
        // vert_prog
        "  gl_TexCoord[1] = vec4(detailScale, 1.0, 1.0) * gl_MultiTexCoord0;\n"
        "\n",
        // frag_def
        "uniform sampler2D detailMap;\n"
        "\n",
        // frag_prog
        "  result *= texture2D(detailMap, gl_TexCoord[1].st);\n"
        "  result.rgb *= 2.0;\n"
        "\n",
    },
    {
        1 << PR_BIT_DIFFUSE_MODULATION,
        // vert_def
        "",
        // vert_prog
        "  gl_FrontColor = gl_Color;\n"
        "\n",
        // frag_def
        "",
        // frag_prog
        "  result *= vec4(gl_Color);\n"
        "\n",
    },
    {
        1 << PR_BIT_POINT_LIGHT,
        // vert_def
        "varying vec3 vertexNormal;\n"
        "varying vec3 vertexPos;\n"
        "\n",
        // vert_prog
        "  vertexNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
        "  vertexPos = vec3(gl_ModelViewMatrix * gl_Vertex);\n"
        "\n",
        // frag_def
        "uniform int lightCount;\n"
        "varying vec3 vertexNormal;\n"
        "varying vec3 vertexPos;\n"
        "\n",
        // frag_prog
        "  vec3 fragmentNormal;\n"
        "  vec3 lightPos;\n"
        "  vec3 lightDiffuse;\n"
        "  vec2 lightRange;\n"
        "  vec3 lightVector;\n"
        "  float dotNormalLightDir;\n"
        "  float lightAttenuation;\n"
        "  float pointLightDistance;\n"
        "\n"
        "  fragmentNormal = normalize(vertexNormal);\n"
        "\n"
        "  while (l < LIGHTCOUNT) {\n"
        "    lightPos = gl_LightSource[l].ambient.rgb;\n"
        "    lightDiffuse = gl_LightSource[l].diffuse.rgb;\n"
        "    lightRange.x = gl_LightSource[l].constantAttenuation;\n"
        "    lightRange.y = gl_LightSource[l].linearAttenuation;\n"
        "\n"
        "    lightVector = lightPos - vertexPos;\n"
        "    pointLightDistance = length(lightVector);\n"
        "    dotNormalLightDir = max(dot(fragmentNormal, normalize(lightVector)), 0.0);\n"
        "    if (pointLightDistance < lightRange.y)\n"
        "    {\n"
        "      if (pointLightDistance < lightRange.x)\n"
        "        lightAttenuation = 1.0;\n"
        "      else {\n"
        "        lightAttenuation = 1.0 - (pointLightDistance - lightRange.x)  /\n"
        "                                 (lightRange.y - lightRange.x);\n"
        "      }\n"
        "      result += vec4(lightAttenuation * dotNormalLightDir * lightDiffuse, 0.0);\n"
        "      float specular = pow( max(dot(reflect(-normalize(lightVector), fragmentNormal), normalize(-vertexPos)), 0.0), 60.0);\n"
        "      result += vec4(lightAttenuation * specular * vec3(1.0, 0.5, 0.5), 0.0);\n"
        "    }\n"
        "\n"
        "    l++;\n"
        "  }\n"
        "\n",
    },
    {
        1 << PR_BIT_DIFFUSE_GLOW_MAP,
        // vert_def
        "",
        // vert_prog
        "  gl_TexCoord[2] = gl_MultiTexCoord0;\n"
        "\n",
        // frag_def
        "uniform sampler2D glowMap;\n"
        "\n",
        // frag_prog
        "  vec4 glowTexel;\n"
        "\n"
        "  glowTexel = texture2D(glowMap, gl_TexCoord[2].st);\n"
        "  result = vec4((result.rgb * (1.0 - glowTexel.a)) + (glowTexel.rgb * glowTexel.a), result.a);\n"
        "\n",
    },
    {
        1 << PR_BIT_FOOTER,
        // vert_def
        "void main(void)\n"
        "{\n"
        "  vec4 result = ftransform();\n"
        "  int l = 0;\n"
        "\n",
        // vert_prog
        "  gl_Position = result;\n"
        "}\n",
        // frag_def
        "void main(void)\n"
        "{\n"
        "  vec4 result = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "  int l = 0;\n"
        "\n",
        // frag_prog
        "  gl_FragColor = result;\n"
        "}\n",
    }
};

_prprograminfo  prprograms[1 << PR_BIT_COUNT];

// CONTROL
GLfloat         spritemodelview[16];
GLfloat         rootmodelviewmatrix[16];
GLfloat         *curmodelviewmatrix;
GLfloat         projectionmatrix[16];

int32_t         updatesectors = 1;

int32_t         depth;
int32_t         mirrorfrom[10]; // -3: no mirror; -2: floor; -1: ceiling; >=0: wallnum

GLUtesselator*  prtess;

int16_t         cursky;

int16_t         viewangle;

int32_t         rootsectnum;

_pranimatespritesinfo asi;

// EXTERNAL FUNCTIONS
int32_t             polymer_init(void)
{
    int32_t         i, j;

    if (pr_verbosity >= 1) OSD_Printf("Initalizing Polymer subsystem...\n");

    i = 0;
    while (i < MAXSECTORS)
    {
        prsectors[i] = NULL;
        i++;
    }

    i = 0;
    while (i < MAXWALLS)
    {
        prwalls[i] = NULL;
        i++;
    }

    prtess = bgluNewTess();
    if (prtess == 0)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Tesselator initialization failed.\n");
        return (0);
    }

    polymer_loadboard();

    polymer_initartsky();

    // init the face sprite modelview to identity
    i = 0;
    while (i < 4)
    {
        j = 0;
        while (j < 4)
        {
            if (i == j)
                spritemodelview[(i * 4) + j] = 1.0;
            else
                spritemodelview[(i * 4) + j] = 0.0;
            j++;
        }
        i++;
    }

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete.\n");
    return (1);
}

void                polymer_glinit(void)
{
    float           a;

    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClearStencil(0);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    bglViewport(0, 0, xdim, ydim);

    // texturing
    bglEnable(GL_TEXTURE_2D);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    bglTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    bglEnable(GL_DEPTH_TEST);
    bglDepthFunc(GL_LEQUAL);

    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bgluPerspective((float)(pr_fov) / (2048.0f / 360.0f), (float)xdim / (float)ydim, 0.1f, 100.0f);

    // get the new projection matrix
    bglGetFloatv(GL_PROJECTION_MATRIX, projectionmatrix);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglDisable(GL_FOG);

    bglFogi(GL_FOG_MODE, GL_EXP2);
    //glFogfv(GL_FOG_COLOR, fogColor);
    bglEnable(GL_FOG);

    a = (1 - ((float)(visibility) / 512.0f)) / 10.0f;
    bglFogf(GL_FOG_DENSITY, 0.1f - a);
    bglFogf(GL_FOG_START, 0.0f);
    bglFogf(GL_FOG_END, 1000000.0f);

    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);
}

void                polymer_loadboard(void)
{
    int32_t         i;

    i = 0;
    while (i < numsectors)
    {
        polymer_initsector(i);
        polymer_updatesector(i);
        i++;
    }

    i = 0;
    while (i < numwalls)
    {
        polymer_initwall(i);
        polymer_updatewall(i);
        i++;
    }

    polymer_getsky();

    if (pr_verbosity >= 1) OSD_Printf("PR : Board loaded.\n");
}

void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum)
{
    int16_t         cursectnum;
    int32_t         i;
    float           ang, horizang, tiltang;
    float           pos[3];

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    ang = (float)(daang) / (2048.0f / 360.0f);
    horizang = (float)(-getangle(128, dahoriz-100)) / (2048.0f / 360.0f);
    tiltang = (gtang * 90.0f);

    pos[0] = daposy;
    pos[1] = -(float)(daposz) / 16.0f;
    pos[2] = -daposx;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglRotatef(tiltang, 0.0f, 0.0f, -1.0f);
    bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

    bglDisable(GL_DEPTH_TEST);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    polymer_drawsky(cursky);
    bglEnable(GL_DEPTH_TEST);

    bglScalef(1.0f / 1000.0f, 1.0f / 1000.0f, 1.0f / 1000.0f);
    bglTranslatef(-pos[0], -pos[1], -pos[2]);

    bglGetFloatv(GL_MODELVIEW_MATRIX, rootmodelviewmatrix);

    cursectnum = dacursectnum;
    updatesector(daposx, daposy, &cursectnum);

    if ((cursectnum >= 0) && (cursectnum < numsectors))
        dacursectnum = cursectnum;

    // unflag all sectors
    i = 0;
    while (i < numsectors)
    {
        prsectors[i]->controlstate = 0;
        prsectors[i]->wallsproffset = 0.0f;
        prsectors[i]->floorsproffset = 0.0f;
        i++;
    }
    i = 0;
    while (i < numwalls)
    {
        prwalls[i]->controlstate = 0;
        i++;
    }

    // external view (editor)
    if ((dacursectnum < 0) || (dacursectnum >= numsectors) ||
            (daposz > sector[dacursectnum].floorz) ||
            (daposz < sector[dacursectnum].ceilingz))
    {
        i = 0;
        while (i < numsectors)
        {
            polymer_updatesector(i);
            polymer_drawsector(i);
            polymer_scansprites(i, tsprite, &spritesortcnt);
            i++;
        }

        i = 0;
        while (i < numwalls)
        {
            polymer_updatewall(i);
            polymer_drawwall(sectorofwall(i), i);
            i++;
        }
        viewangle = daang;
        return;
    }

    rootsectnum = dacursectnum;

    // GO!
    depth = 0;
    mirrorfrom[0] = -3; // no mirror
    polymer_displayrooms(dacursectnum);

    curmodelviewmatrix = rootmodelviewmatrix;

    // build globals used by rotatesprite
    viewangle = daang;
    globalang = (daang&2047);
    cosglobalang = sintable[(globalang+512)&2047];
    singlobalang = sintable[globalang&2047];
    cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
    sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);

    // polymost globals used by polymost_dorotatesprite
    gcosang = ((double)cosglobalang)/262144.0;
    gsinang = ((double)singlobalang)/262144.0;
    gcosang2 = gcosang*((double)viewingrange)/65536.0;
    gsinang2 = gsinang*((double)viewingrange)/65536.0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
}

void                polymer_drawmasks(void)
{
    bglEnable(GL_ALPHA_TEST);
    bglEnable(GL_BLEND);
    bglEnable(GL_POLYGON_OFFSET_FILL);

    while (spritesortcnt)
    {
        spritesortcnt--;
        tspriteptr[spritesortcnt] = &tsprite[spritesortcnt];
        polymer_drawsprite(spritesortcnt);
    }

    bglDisable(GL_POLYGON_OFFSET_FILL);
    bglDisable(GL_BLEND);
    bglDisable(GL_ALPHA_TEST);
}

void                polymer_rotatesprite(int32_t sx, int32_t sy, int32_t z, int16_t a, int16_t picnum, int8_t dashade, char dapalnum, char dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
    UNREFERENCED_PARAMETER(sx);
    UNREFERENCED_PARAMETER(sy);
    UNREFERENCED_PARAMETER(z);
    UNREFERENCED_PARAMETER(a);
    UNREFERENCED_PARAMETER(picnum);
    UNREFERENCED_PARAMETER(dashade);
    UNREFERENCED_PARAMETER(dapalnum);
    UNREFERENCED_PARAMETER(dastat);
    UNREFERENCED_PARAMETER(cx1);
    UNREFERENCED_PARAMETER(cy1);
    UNREFERENCED_PARAMETER(cx2);
    UNREFERENCED_PARAMETER(cy2);
}

void                polymer_drawmaskwall(int32_t damaskwallcnt)
{
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);

    w = prwalls[maskwall[damaskwallcnt]];

    polymer_drawplane(-1, -3, &w->mask, 0);
}

void                polymer_drawsprite(int32_t snum)
{
    int32_t         curpicnum, xsize, ysize, tilexoff, tileyoff, xoff, yoff;
    spritetype      *tspr;
    float           xratio, yratio, ang;
    float           spos[3];

    if (pr_verbosity >= 3) OSD_Printf("PR : Sprite %i...\n", snum);

    tspr = tspriteptr[snum];

    if (usemodels && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].modelid >= 0 && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].framenum >= 0)
    {
        polymer_drawmdsprite(tspr);
        return;
    }

    curpicnum = tspr->picnum;
    if (picanm[curpicnum]&192) curpicnum += animateoffs(curpicnum,tspr->owner+32768);

    polymer_getbuildmaterial(&spriteplane.material, curpicnum, tspr->pal, tspr->shade);

    if (tspr->cstat & 2)
    {
        if (tspr->cstat & 512)
            spriteplane.material.diffusemodulation[3] = 0.33f;
        else
            spriteplane.material.diffusemodulation[3] = 0.66f;
    }

    if (((tspr->cstat>>4) & 3) == 0)
        xratio = (float)(tspr->xrepeat) * 32.0f / 160.0f;
    else
        xratio = (float)(tspr->xrepeat) / 4.0f;

    yratio = (float)(tspr->yrepeat) / 4.0f;

    if (usehightile && h_xsize[curpicnum])
    {
        xsize = h_xsize[curpicnum];
        ysize = h_ysize[curpicnum];
    } else {
        xsize = tilesizx[curpicnum];
        ysize = tilesizy[curpicnum];
    }

    xsize *= xratio;
    ysize *= yratio;

    tilexoff = (int32_t)tspr->xoffset;
    tileyoff = (int32_t)tspr->yoffset;
    tilexoff += (int8_t)((usehightile&&h_xsize[curpicnum])?(h_xoffs[curpicnum]):((picanm[curpicnum]>>8)&255));
    tileyoff += (int8_t)((usehightile&&h_xsize[curpicnum])?(h_yoffs[curpicnum]):((picanm[curpicnum]>>16)&255));

    xoff = tilexoff * xratio;
    yoff = tileyoff * yratio;

    if (tspr->cstat & 128)
        yoff -= ysize / 2;

    spos[0] = tspr->y;
    spos[1] = -(float)(tspr->z) / 16.0f;
    spos[2] = -tspr->x;

    spriteplane.buffer = vertsprite;

    if (pr_billboardingmode && !((tspr->cstat>>4) & 3))
    {
        // do surgery on the face tspr to make it look like a wall sprite
        tspr->cstat |= 16;
        tspr->ang = (viewangle + 1024) & 2047;
    }

    switch ((tspr->cstat>>4) & 3)
    {
    case 0:
        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();

        spritemodelview[12] =   curmodelviewmatrix[0] * spos[0] +
                                curmodelviewmatrix[4] * spos[1] +
                                curmodelviewmatrix[8] * spos[2] +
                                curmodelviewmatrix[12];
        spritemodelview[13] =   curmodelviewmatrix[1] * spos[0] +
                                curmodelviewmatrix[5] * spos[1] +
                                curmodelviewmatrix[9] * spos[2] +
                                curmodelviewmatrix[13];
        spritemodelview[14] =   curmodelviewmatrix[2]  * spos[0] +
                                curmodelviewmatrix[6]  * spos[1] +
                                curmodelviewmatrix[10] * spos[2] +
                                curmodelviewmatrix[14];

        bglLoadMatrixf(spritemodelview);
        bglRotatef((gtang * 90.0f), 0.0f, 0.0f, -1.0f);
        bglTranslatef((float)(-xoff)/1000.0f, (float)(yoff)/1000.0f, 0.0f);
        bglScalef((float)(xsize) / 1000.0f, (float)(ysize) / 1000.0f, 1.0f / 1000.0f);

        bglPolygonOffset(0.0f, 0.0f);
        break;
    case 1:
        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();

        ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglTranslatef((float)(-xoff), (float)(yoff), 0.0f);
        bglScalef((float)(xsize), (float)(ysize), 1.0f);

        prsectors[tspr->sectnum]->wallsproffset += 0.5f;
        bglPolygonOffset(-prsectors[tspr->sectnum]->wallsproffset,
                         -prsectors[tspr->sectnum]->wallsproffset);
        break;
    case 2:
        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();

        ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglTranslatef((float)(-xoff), 1.0f, (float)(yoff));
        bglScalef((float)(xsize), 1.0f, (float)(ysize));

        spriteplane.buffer = horizsprite;

        prsectors[tspr->sectnum]->floorsproffset += 0.5f;
        bglPolygonOffset(-prsectors[tspr->sectnum]->floorsproffset,
                         -prsectors[tspr->sectnum]->floorsproffset);
        break;
    }

    if ((tspr->cstat & 4) || (((tspr->cstat>>4) & 3) == 2))
        spriteplane.material.diffusescale[0] = -spriteplane.material.diffusescale[0];

    if (tspr->cstat & 8)
        spriteplane.material.diffusescale[1] = -spriteplane.material.diffusescale[1];

    if ((tspr->cstat & 64) && (((tspr->cstat>>4) & 3) == 1))
        bglEnable(GL_CULL_FACE);

//     bglEnable(GL_POLYGON_OFFSET_FILL);

    polymer_drawplane(-1, -3, &spriteplane, 0);

//     bglDisable(GL_POLYGON_OFFSET_FILL);

    if ((tspr->cstat & 64) && (((tspr->cstat>>4) & 3) == 1))
        bglDisable(GL_CULL_FACE);

    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);
    bglPopMatrix();
}

void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio)
{
    asi.animatesprites = animatesprites;
    asi.x = x;
    asi.y = y;
    asi.a = a;
    asi.smoothratio = smoothratio;
}

void                polymer_resetlights(void)
{
    lightcount = 0;
}

void                polymer_addlight(_prlight light)
{
    if (lightcount < PR_MAXLIGHTS)
        prlights[lightcount++] = light;
}

// CORE
static void         polymer_displayrooms(int16_t dacursectnum)
{
    sectortype      *sec, *nextsec;
    walltype        *wal, *nextwal;
    _prwall         *w;
    int32_t         i, j;
    GLint           result;
    int32_t         front;
    int32_t         back;
    int32_t         firstback;
    int16_t         sectorqueue[MAXSECTORS];
    int16_t         querydelay[MAXSECTORS];
    GLuint          queryid[MAXSECTORS];
    int16_t         drawingstate[MAXSECTORS];
    GLfloat         localmodelviewmatrix[16];
    float           frustum[5 * 4];
    int32_t         localspritesortcnt;
    spritetype      localtsprite[MAXSPRITESONSCREEN];
    int16_t         localmaskwall[MAXWALLSB], localmaskwallcnt;

    if (depth)
    {
        curmodelviewmatrix = localmodelviewmatrix;
        bglGetFloatv(GL_MODELVIEW_MATRIX, localmodelviewmatrix);
    }
    else
        curmodelviewmatrix = rootmodelviewmatrix;

    polymer_extractfrustum(curmodelviewmatrix, projectionmatrix, frustum);

    memset(querydelay, 0, sizeof(int16_t) * MAXSECTORS);
    memset(queryid, 0, sizeof(GLuint) * MAXSECTORS);
    memset(drawingstate, 0, sizeof(int16_t) * MAXSECTORS);

    front = 0;
    back = 0;
    localspritesortcnt = localmaskwallcnt = 0;

    polymer_pokesector(dacursectnum);
    polymer_drawsector(dacursectnum);
    polymer_scansprites(dacursectnum, localtsprite, &localspritesortcnt);
    drawingstate[dacursectnum] = 1;

    sec = &sector[dacursectnum];
    wal = &wall[sec->wallptr];

    i = 0;
    while (i < sec->wallnum)
    {
        if (((wallvisible(sec->wallptr + i))) &&
                (polymer_portalinfrustum(sec->wallptr + i, frustum)))
        {
            if (mirrorfrom[depth] != (sec->wallptr + i))
                polymer_drawwall(dacursectnum, sec->wallptr + i);
            // mask
            if ((wal->cstat&48) == 16) localmaskwall[localmaskwallcnt++] = sec->wallptr + i;

            if ((wal->nextsector != -1) &&
                    (drawingstate[wal->nextsector] == 0))
            {
                sectorqueue[back++] = wal->nextsector;
                drawingstate[wal->nextsector] = 1;
            }
        }
        i++;
        wal = &wall[sec->wallptr + i];
    }

    mirrorfrom[depth] = -3;

    firstback = back;

    while (front != back)
    {
        if ((front >= firstback) && (pr_occlusionculling))
        {
            if (querydelay[sectorqueue[front]] == 0)
            {
                bglGetQueryObjectivARB(queryid[sectorqueue[front]],
                                       GL_QUERY_RESULT_ARB,
                                       &result);
                bglDeleteQueriesARB(1, &queryid[sectorqueue[front]]);
                if (!result)
                {
                    front++;
                    continue;
                }
                else
                    querydelay[sectorqueue[front]] = pr_occlusionculling-1;
            }
            else if (querydelay[sectorqueue[front]] == -1)
                querydelay[sectorqueue[front]] = pr_occlusionculling-1;
            else if (querydelay[sectorqueue[front]])
                querydelay[sectorqueue[front]]--;
        }

        polymer_pokesector(sectorqueue[front]);
        polymer_drawsector(sectorqueue[front]);
        polymer_scansprites(sectorqueue[front], localtsprite, &localspritesortcnt);

        // scan sectors
        sec = &sector[sectorqueue[front]];
        wal = &wall[sec->wallptr];

        i = 0;
        while (i < sec->wallnum)
        {
            if ((wallvisible(sec->wallptr + i)) &&
                    (polymer_portalinfrustum(sec->wallptr + i, frustum)))
            {
                polymer_drawwall(sectorqueue[front], sec->wallptr + i);
                // mask
                if ((wal->cstat&48) == 16) localmaskwall[localmaskwallcnt++] = sec->wallptr + i;

                if ((wal->nextsector != -1) &&
                        (drawingstate[wal->nextsector] == 0))
                {
                    polymer_pokesector(wal->nextsector);
                    sectorqueue[back++] = wal->nextsector;
                    drawingstate[wal->nextsector] = 1;

                    if (pr_occlusionculling && !querydelay[wal->nextsector])
                    {
                        nextsec = &sector[wal->nextsector];
                        nextwal = &wall[nextsec->wallptr];

                        if ((nextsec->ceilingstat & 1) &&
                                (nextsec->floorz == nextsec->ceilingz))
                        {
                            querydelay[wal->nextsector] = -1;
                            i++;
                            wal = &wall[sec->wallptr + i];
                            continue;
                        }

                        bglDisable(GL_TEXTURE_2D);
                        bglDisable(GL_FOG);
                        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                        bglDepthMask(GL_FALSE);

                        bglGenQueriesARB(1, &queryid[wal->nextsector]);
                        bglBeginQueryARB(GL_SAMPLES_PASSED_ARB, queryid[wal->nextsector]);

                        j = 0;
                        while (j < nextsec->wallnum)
                        {
                            if ((nextwal->nextwall == (sec->wallptr + i)) ||
                                    ((nextwal->nextwall != -1) &&
                                     (wallvisible(nextwal->nextwall)) &&
                                     (polymer_portalinfrustum(nextwal->nextwall, frustum))))
                            {
                                w = prwalls[nextwal->nextwall];

                                if (pr_vbos > 0)
                                {
                                    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
                                    bglVertexPointer(3, GL_FLOAT, 0, NULL);
                                }
                                else
                                    bglVertexPointer(3, GL_FLOAT, 0, w->bigportal);

                                bglDrawArrays(GL_QUADS, 0, 4);

                                if (pr_vbos > 0)
                                    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
                            }

                            j++;
                            nextwal = &wall[nextsec->wallptr + j];
                        }
                        bglEndQueryARB(GL_SAMPLES_PASSED_ARB);

                        bglDepthMask(GL_TRUE);
                        bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                        bglEnable(GL_FOG);
                        bglEnable(GL_TEXTURE_2D);
                    }
                }
            }
            i++;
            wal = &wall[sec->wallptr + i];
        }
        front++;
    }

    spritesortcnt = localspritesortcnt;
    memcpy(tsprite, localtsprite, sizeof(spritetype) * MAXSPRITESONSCREEN);
    maskwallcnt = localmaskwallcnt;
    memcpy(maskwall, localmaskwall, sizeof(int16_t) * MAXWALLSB);

    if (depth)
    {
        cosglobalang = sintable[(viewangle+512)&2047];
        singlobalang = sintable[viewangle&2047];
        cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
        sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);

        display_mirror = 1;
        polymer_animatesprites();
        display_mirror = 0;

        bglDisable(GL_CULL_FACE);
        drawmasks();
        bglEnable(GL_CULL_FACE);
        bglEnable(GL_BLEND);
    }
}

#define OMGDRAWSHITVBO                                                                      \
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, plane->vbo);                                      \
    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), NULL);                               \
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), (GLfloat*)(3 * sizeof(GLfloat)));  \
    if (!plane->indices)                                                                    \
        bglDrawArrays(GL_QUADS, 0, 4);                                                      \
    else                                                                                    \
    {                                                                                       \
        bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, plane->ivbo);                         \
        bglDrawElements(GL_TRIANGLES, indicecount, GL_UNSIGNED_SHORT, NULL);                \
    }                                                                                       \
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);                                               \
    bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0)

#define OMGDRAWSHIT                                                                     \
    bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), plane->buffer);                  \
    bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &plane->buffer[3]);            \
    if (!plane->indices)                                                                \
        bglDrawArrays(GL_QUADS, 0, 4);                                                  \
    else                                                                                \
        bglDrawElements(GL_TRIANGLES, indicecount, GL_UNSIGNED_SHORT, plane->indices)

static void         polymer_drawplane(int16_t sectnum, int16_t wallnum, _prplane* plane, int32_t indicecount)
{
    int32_t         materialbits;

//     if ((depth < 1) && (plane != NULL) &&
//             (wallnum >= 0) && (wall[wallnum].overpicnum == 560)) // insert mirror condition here
//     {
//         int32_t         gx, gy, gz, px, py, pz;
//         float           coeff;
// 
//         // set the stencil to 1 and clear the area to black where the sector floor is
//         bglDisable(GL_TEXTURE_2D);
//         bglDisable(GL_FOG);
//         bglColor4f(0.0f, 1.0f, 0.0f, 1.0f);
//         bglDepthMask(GL_FALSE);
// 
//         bglEnable(GL_STENCIL_TEST);
//         bglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
//         bglStencilFunc(GL_EQUAL, 0, 0xffffffff);
// 
//         if (plane->vbo && (pr_vbos > 0))
//         {
//             OMGDRAWSHITVBO;
//         }
//         else
//         {
//             OMGDRAWSHIT;
//         }
// 
//         bglDepthMask(GL_TRUE);
// 
//         // set the depth to 1 where we put the stencil by drawing a screen aligned quad
//         bglStencilFunc(GL_EQUAL, 1, 0xffffffff);
//         bglStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
//         bglDepthFunc(GL_ALWAYS);
//         bglMatrixMode(GL_PROJECTION);
//         bglPushMatrix();
//         bglLoadIdentity();
//         bglMatrixMode(GL_MODELVIEW);
//         bglPushMatrix();
//         bglLoadIdentity();
// 
//         bglColor4f(0.0f, 0.0f, 0.0f, 1.0f);
//         bglBegin(GL_QUADS);
//         bglVertex3f(-1.0f, -1.0f, 1.0f);
//         bglVertex3f(1.0f, -1.0f, 1.0f);
//         bglVertex3f(1.0f, 1.0f, 1.0f);
//         bglVertex3f(-1.0f, 1.0f, 1.0f);
//         bglEnd();
// 
//         bglMatrixMode(GL_PROJECTION);
//         bglPopMatrix();
//         bglMatrixMode(GL_MODELVIEW);
//         bglPopMatrix();
//         bglDepthFunc(GL_LEQUAL);
//         bglEnable(GL_TEXTURE_2D);
//         bglEnable(GL_FOG);
//         // finally draw the shit
//         bglPushMatrix();
//         bglClipPlane(GL_CLIP_PLANE0, plane->plane);
//         polymer_inb4mirror(plane->buffer, plane->plane);
//         bglCullFace(GL_FRONT);
//         bglEnable(GL_CLIP_PLANE0);
// 
//         if (wallnum >= 0)
//             preparemirror(globalposx, globalposy, 0, globalang,
//                           0, wallnum, 0, &gx, &gy, &viewangle);
// 
//         gx = globalposx;
//         gy = globalposy;
//         gz = globalposz;
// 
//         // map the player pos from build to polymer
//         px = globalposy;
//         py = -globalposz / 16;
//         pz = -globalposx;
// 
//         // calculate new player position on the other side of the mirror
//         // this way the basic build visibility shit can be used (wallvisible)
//         coeff = -plane->plane[0] * px +
//                 -plane->plane[1] * py +
//                 -plane->plane[2] * pz +
//                 -plane->plane[3];
// 
//         coeff /= (float)(plane->plane[0] * plane->plane[0] +
//                          plane->plane[1] * plane->plane[1] +
//                          plane->plane[2] * plane->plane[2]);
// 
//         px = coeff*plane->plane[0]*2 + px;
//         py = coeff*plane->plane[1]*2 + py;
//         pz = coeff*plane->plane[2]*2 + pz;
// 
//         // map back from polymer to build
//         globalposx = -pz;
//         globalposy = px;
//         globalposz = -py * 16;
// 
//         depth++;
//         mirrorfrom[depth] = wallnum;
//         polymer_displayrooms(sectnum);
//         depth--;
// 
//         globalposx = gx;
//         globalposy = gy;
//         globalposz = gz;
// 
//         bglDisable(GL_CLIP_PLANE0);
//         bglCullFace(GL_BACK);
//         bglMatrixMode(GL_MODELVIEW);
//         bglPopMatrix();
// 
//         bglColor4f(plane->material.diffusemodulation[0],
//                    plane->material.diffusemodulation[1],
//                    plane->material.diffusemodulation[2],
//                    0.0f);
//     }
//     else
//         bglColor4f(plane->material.diffusemodulation[0],
//                    plane->material.diffusemodulation[1],
//                    plane->material.diffusemodulation[2],
//                    plane->material.diffusemodulation[3]);

//     bglBindTexture(GL_TEXTURE_2D, plane->material.diffusemap);

    bglNormal3f((float)(-plane->plane[0]), (float)(-plane->plane[1]), (float)(-plane->plane[2]));

    materialbits = polymer_bindmaterial(plane->material);

    if (plane->vbo && (pr_vbos > 0))
    {
        OMGDRAWSHITVBO;
    }
    else
    {
        OMGDRAWSHIT;
    }

    polymer_unbindmaterial(materialbits);

//     if ((depth < 1) && (plane->plane != NULL) &&
//             (wallnum >= 0) && (wall[wallnum].overpicnum == 560)) // insert mirror condition here
//     {
//         bglDisable(GL_STENCIL_TEST);
//         bglClear(GL_STENCIL_BUFFER_BIT);
//     }
}

static void         polymer_inb4mirror(GLfloat* buffer, GLdouble* plane)
{
    float           pv;
    float           reflectionmatrix[16];

    pv = buffer[0] * plane[0] +
         buffer[1] * plane[1] +
         buffer[2] * plane[2];

    reflectionmatrix[0] = 1 - (2 * plane[0] * plane[0]);
    reflectionmatrix[1] = -2 * plane[0] * plane[1];
    reflectionmatrix[2] = -2 * plane[0] * plane[2];
    reflectionmatrix[3] = 0;

    reflectionmatrix[4] = -2 * plane[0] * plane[1];
    reflectionmatrix[5] = 1 - (2 * plane[1] * plane[1]);
    reflectionmatrix[6] = -2 * plane[1] * plane[2];
    reflectionmatrix[7] = 0;

    reflectionmatrix[8] = -2 * plane[0] * plane[2];
    reflectionmatrix[9] = -2 * plane[1] * plane[2];
    reflectionmatrix[10] = 1 - (2 * plane[2] * plane[2]);
    reflectionmatrix[11] = 0;

    reflectionmatrix[12] = 2 * pv * plane[0];
    reflectionmatrix[13] = 2 * pv * plane[1];
    reflectionmatrix[14] = 2 * pv * plane[2];
    reflectionmatrix[15] = 1;

    bglMultMatrixf(reflectionmatrix);
}

static void         polymer_animatesprites(void)
{
    if (asi.animatesprites)
        asi.animatesprites(globalposx, globalposy, viewangle, asi.smoothratio);
}

// SECTORS
static int32_t      polymer_initsector(int16_t sectnum)
{
    sectortype      *sec;
    _prsector*      s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = calloc(1, sizeof(_prsector));
    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize sector %i : malloc failed.\n", sectnum);
        return (0);
    }

    s->verts = calloc(sec->wallnum, sizeof(GLdouble) * 3);
    s->floor.buffer = calloc(sec->wallnum, sizeof(GLfloat) * 5);
    s->ceil.buffer = calloc(sec->wallnum, sizeof(GLfloat) * 5);
    if ((s->verts == NULL) || (s->floor.buffer == NULL) || (s->ceil.buffer == NULL))
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize geometry of sector %i : malloc failed.\n", sectnum);
        return (0);
    }
    bglGenBuffersARB(1, &s->floor.vbo);
    bglGenBuffersARB(1, &s->ceil.vbo);
    bglGenBuffersARB(1, &s->floor.ivbo);
    bglGenBuffersARB(1, &s->ceil.ivbo);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->floor.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, sec->wallnum * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->ceil.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, sec->wallnum * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    s->controlstate = 2; // let updatesector know that everything needs to go

    prsectors[sectnum] = s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized sector %i.\n", sectnum);

    return (1);
}

static int32_t      polymer_updatesector(int16_t sectnum)
{
    _prsector*      s;
    sectortype      *sec;
    walltype        *wal;
    int32_t         i, j;
    int32_t         ceilz, florz;
    int32_t         tex, tey, heidiff;
    float           secangcos, secangsin, scalecoef, xpancoef, ypancoef;
    int32_t         ang, needfloor, wallinvalidate;
    int16_t         curstat, curpicnum, floorpicnum, ceilingpicnum;
    char            curxpanning, curypanning;
    GLfloat*        curbuffer;

    s = prsectors[sectnum];
    sec = &sector[sectnum];

    secangcos = secangsin = 2;

    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Can't update uninitialized sector %i.\n", sectnum);
        return (-1);
    }

    needfloor = wallinvalidate = 0;

    // geometry
    wal = &wall[sec->wallptr];
    i = 0;
    while (i < sec->wallnum)
    {
        if ((-wal->x != s->verts[(i*3)+2]))
        {
            s->verts[(i*3)+2] = s->floor.buffer[(i*5)+2] = s->ceil.buffer[(i*5)+2] = -wal->x;
            needfloor = wallinvalidate = 1;
        }
        if ((wal->y != s->verts[i*3]))
        {
            s->verts[i*3] = s->floor.buffer[i*5] = s->ceil.buffer[i*5] = wal->y;
            needfloor = wallinvalidate = 1;
        }

        i++;
        wal = &wall[sec->wallptr + i];
    }

    if ((s->controlstate == 2) ||
            (sec->floorz != s->floorz) ||
            (sec->ceilingz != s->ceilingz) ||
            (sec->floorheinum != s->floorheinum) ||
            (sec->ceilingheinum != s->ceilingheinum))
    {
        wallinvalidate = 1;

        wal = &wall[sec->wallptr];
        i = 0;
        while (i < sec->wallnum)
        {
            getzsofslope(sectnum, wal->x, wal->y, &ceilz, &florz);
            s->floor.buffer[(i*5)+1] = -(float)(florz) / 16.0f;
            s->ceil.buffer[(i*5)+1] = -(float)(ceilz) / 16.0f;

            i++;
            wal = &wall[sec->wallptr + i];
        }

        s->floorz = sec->floorz;
        s->ceilingz = sec->ceilingz;
        s->floorheinum = sec->floorheinum;
        s->ceilingheinum = sec->ceilingheinum;
    }

    floorpicnum = sec->floorpicnum;
    if (picanm[floorpicnum]&192) floorpicnum += animateoffs(floorpicnum,sectnum);
    ceilingpicnum = sec->ceilingpicnum;
    if (picanm[ceilingpicnum]&192) ceilingpicnum += animateoffs(ceilingpicnum,sectnum);

    if ((s->controlstate != 2) && (!needfloor) &&
            (sec->floorstat == s->floorstat) &&
            (sec->ceilingstat == s->ceilingstat) &&
            (floorpicnum == s->floorpicnum) &&
            (ceilingpicnum == s->ceilingpicnum) &&
            (sec->floorxpanning == s->floorxpanning) &&
            (sec->ceilingxpanning == s->ceilingxpanning) &&
            (sec->floorypanning == s->floorypanning) &&
            (sec->ceilingypanning == s->ceilingypanning))
        goto attributes;

    wal = &wall[sec->wallptr];
    i = 0;
    while (i < sec->wallnum)
    {
        j = 2;
        curstat = sec->floorstat;
        curbuffer = s->floor.buffer;
        curpicnum = floorpicnum;
        curxpanning = sec->floorxpanning;
        curypanning = sec->floorypanning;

        while (j)
        {
            if (j == 1)
            {
                curstat = sec->ceilingstat;
                curbuffer = s->ceil.buffer;
                curpicnum = ceilingpicnum;
                curxpanning = sec->ceilingxpanning;
                curypanning = sec->ceilingypanning;
            }

            if (!waloff[curpicnum])
                loadtile(curpicnum);

            if (((sec->floorstat & 64) || (sec->ceilingstat & 64)) &&
                    ((secangcos == 2) && (secangsin == 2)))
            {
                ang = (getangle(wall[wal->point2].x - wal->x, wall[wal->point2].y - wal->y) + 512) & 2047;
                secangcos = (float)(sintable[(ang+512)&2047]) / 16383.0f;
                secangsin = (float)(sintable[ang&2047]) / 16383.0f;
            }


            tex = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangsin) + ((-wal->y - -wall[sec->wallptr].y) * secangcos) : wal->x;
            tey = (curstat & 64) ? ((wal->x - wall[sec->wallptr].x) * secangcos) - ((wall[sec->wallptr].y - wal->y) * secangsin) : -wal->y;

            if ((curstat & (2+64)) == (2+64))
            {
                heidiff = curbuffer[(i*5)+1] - curbuffer[1];
                tey = sqrt((tey * tey) + (heidiff * heidiff));
            }

            if (curstat & 4)
                swaplong(&tex, &tey);

            if (curstat & 16) tex = -tex;
            if (curstat & 32) tey = -tey;

            scalecoef = (curstat & 8) ? 8.0f : 16.0f;

            if (curxpanning)
            {
                xpancoef = (float)(pow2long[picsiz[curpicnum] & 15]);
                xpancoef *= (float)(curxpanning) / (256.0f * (float)(tilesizx[curpicnum]));
            }
            else
                xpancoef = 0;

            if (curypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                ypancoef *= (float)(curypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            curbuffer[(i*5)+3] = ((float)(tex) / (scalecoef * tilesizx[curpicnum])) + xpancoef;
            curbuffer[(i*5)+4] = ((float)(tey) / (scalecoef * tilesizy[curpicnum])) + ypancoef;

            j--;
        }
        i++;
        wal = &wall[sec->wallptr + i];
    }

    s->floorstat = sec->floorstat;
    s->ceilingstat = sec->ceilingstat;
    s->floorxpanning = sec->floorxpanning;
    s->ceilingxpanning = sec->ceilingxpanning;
    s->floorypanning = sec->floorypanning;
    s->ceilingypanning = sec->ceilingypanning;

    i = -1;

attributes:
    if ((pr_vbos > 0) && ((i == -1) || (wallinvalidate)))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->floor.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sec->wallnum * sizeof(GLfloat) * 5, s->floor.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->ceil.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sec->wallnum * sizeof(GLfloat) * 5, s->ceil.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    if ((s->controlstate != 2) &&
            (sec->floorshade == s->floorshade) &&
            (sec->floorpal == s->floorpal) &&
            (floorpicnum == s->floorpicnum) &&
            (ceilingpicnum == s->ceilingpicnum))
        goto finish;

    polymer_getbuildmaterial(&s->floor.material, floorpicnum, sec->floorpal, sec->floorshade);
    polymer_getbuildmaterial(&s->ceil.material, ceilingpicnum, sec->ceilingpal, sec->ceilingshade);

    s->floorshade = sec->floorshade;
    s->floorpal = sec->floorpal;
    s->floorpicnum = floorpicnum;
    s->ceilingpicnum = ceilingpicnum;

finish:

    if (needfloor)
    {
        polymer_buildfloor(sectnum);
        if ((pr_vbos > 0))
        {
            if (s->oldindicescount < s->indicescount)
            {
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->floor.ivbo);
                bglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->indicescount * sizeof(GLushort), NULL, mapvbousage);
                bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->ceil.ivbo);
                bglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->indicescount * sizeof(GLushort), NULL, mapvbousage);
                s->oldindicescount = s->indicescount;
            }
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->floor.ivbo);
            bglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, s->indicescount * sizeof(GLushort), s->floor.indices);
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->ceil.ivbo);
            bglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, s->indicescount * sizeof(GLushort), s->ceil.indices);
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }
    }

    if (wallinvalidate)
    {
        s->invalidid++;
        polymer_buffertoplane(s->floor.buffer, s->floor.indices, s->indicescount, s->floor.plane);
        polymer_buffertoplane(s->ceil.buffer, s->ceil.indices, s->indicescount, s->ceil.plane);
    }

    s->controlstate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated sector %i.\n", sectnum);

    return (0);
}

void PR_CALLBACK    polymer_tesserror(GLenum error)
{
    // This callback is called by the tesselator whenever it raises an error.
    if (pr_verbosity >= 1) OSD_Printf("PR : Tesselation error number %i reported : %s.\n", error, bgluErrorString(errno));
}

void PR_CALLBACK    polymer_tessedgeflag(GLenum error)
{
    // Passing an edgeflag callback forces the tesselator to output a triangle list
    UNREFERENCED_PARAMETER(error);
    return;
}

void PR_CALLBACK    polymer_tessvertex(void* vertex, void* sector)
{
    _prsector*      s;

    s = (_prsector*)sector;

    if (s->curindice >= s->indicescount)
    {
        if (pr_verbosity >= 2) OSD_Printf("PR : Indice overflow, extending the indices list... !\n");
        s->indicescount++;
        s->floor.indices = realloc(s->floor.indices, s->indicescount * sizeof(GLushort));
        s->ceil.indices = realloc(s->ceil.indices, s->indicescount * sizeof(GLushort));
    }
    s->ceil.indices[s->curindice] = (intptr_t)vertex;
    s->curindice++;
}

static int32_t      polymer_buildfloor(int16_t sectnum)
{
    // This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
    _prsector*      s;
    sectortype      *sec;
    intptr_t        i;

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselating floor of sector %i...\n", sectnum);

    s = prsectors[sectnum];
    sec = &sector[sectnum];

    if (s == NULL)
        return (-1);

    if (s->floor.indices == NULL)
    {
        s->indicescount = (sec->wallnum - 2) * 3;
        s->floor.indices = calloc(s->indicescount, sizeof(GLushort));
        s->ceil.indices = calloc(s->indicescount, sizeof(GLushort));
    }

    s->curindice = 0;

    bgluTessCallback(prtess, GLU_TESS_VERTEX_DATA, polymer_tessvertex);
    bgluTessCallback(prtess, GLU_TESS_EDGE_FLAG, polymer_tessedgeflag);
    bgluTessCallback(prtess, GLU_TESS_ERROR, polymer_tesserror);

    bgluTessProperty(prtess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_POSITIVE);

    bgluTessBeginPolygon(prtess, s);
    bgluTessBeginContour(prtess);

    i = 0;
    while (i < sec->wallnum)
    {
        bgluTessVertex(prtess, s->verts + (3 * i), (void *)i);
        if ((i != (sec->wallnum - 1)) && ((sec->wallptr + i) > wall[sec->wallptr + i].point2))
        {
            bgluTessEndContour(prtess);
            bgluTessBeginContour(prtess);
        }
        i++;
    }
    bgluTessEndContour(prtess);
    bgluTessEndPolygon(prtess);

    i = 0;
    while (i < s->indicescount)
    {
        s->floor.indices[s->indicescount - i - 1] = s->ceil.indices[i];

        i++;
    }

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselated floor of sector %i.\n", sectnum);

    return (1);
}

static void         polymer_drawsector(int16_t sectnum)
{
    sectortype      *sec;
    _prsector*      s;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = prsectors[sectnum];

    if (!(sec->floorstat & 1) && (mirrorfrom[depth] != -2))
        polymer_drawplane(sectnum, -2, &s->floor, s->indicescount);
    if (!(sec->ceilingstat & 1) && (mirrorfrom[depth] != -1))
        polymer_drawplane(sectnum, -1, &s->ceil, s->indicescount);

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

// WALLS
static int32_t      polymer_initwall(int16_t wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing wall %i...\n", wallnum);

    w = calloc(1, sizeof(_prwall));
    if (w == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize wall %i : malloc failed.\n", wallnum);
        return (0);
    }

    if (w->mask.buffer == NULL)
        w->mask.buffer = calloc(4, sizeof(GLfloat) * 5);
    if (w->bigportal == NULL)
        w->bigportal = calloc(4, sizeof(GLfloat) * 3);
    if (w->cap == NULL)
        w->cap = calloc(4, sizeof(GLfloat) * 3);

    bglGenBuffersARB(1, &w->wall.vbo);
    bglGenBuffersARB(1, &w->over.vbo);
    bglGenBuffersARB(1, &w->mask.vbo);
    bglGenBuffersARB(1, &w->stuffvbo);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->wall.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->over.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->mask.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 8 * sizeof(GLfloat) * 3, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    w->controlstate = 2;

    prwalls[wallnum] = w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalized wall %i.\n", wallnum);

    return (1);
}

static void         polymer_updatewall(int16_t wallnum)
{
    int16_t         nwallnum, nnwallnum, curpicnum, wallpicnum, walloverpicnum, nwallpicnum;
    char            curxpanning, curypanning, underwall, overwall, curpal;
    int8_t          curshade;
    walltype        *wal;
    sectortype      *sec, *nsec;
    _prwall         *w;
    _prsector       *s, *ns;
    int32_t         xref, yref;
    float           ypancoef, dist;
    int32_t         i;
    uint32_t        invalid;

    // yes, this function is messy and unefficient
    // it also works, bitches
    wal = &wall[wallnum];
    nwallnum = wal->nextwall;
    sec = &sector[sectorofwall(wallnum)];
    w = prwalls[wallnum];
    s = prsectors[sectorofwall(wallnum)];
    invalid = s->invalidid;
    if (nwallnum != -1)
    {
        ns = prsectors[wal->nextsector];
        invalid += ns->invalidid;
        nsec = &sector[wal->nextsector];
    }
    else
    {
        ns = NULL;
        nsec = NULL;
    }

    if (w->wall.buffer == NULL)
        w->wall.buffer = calloc(4, sizeof(GLfloat) * 5);

    wallpicnum = wal->picnum;
    if (picanm[wallpicnum]&192) wallpicnum += animateoffs(wallpicnum,wallnum+16384);
    walloverpicnum = wal->overpicnum;
    if (picanm[walloverpicnum]&192) walloverpicnum += animateoffs(walloverpicnum,wallnum+16384);
    if (nwallnum != -1)
    {
        nwallpicnum = wall[nwallnum].picnum;
        if (picanm[nwallpicnum]&192) nwallpicnum += animateoffs(nwallpicnum,wallnum+16384);
    }
    else
        nwallpicnum = 0;

    if ((w->controlstate != 2) &&
            (w->invalidid == invalid) &&
            (wal->cstat == w->cstat) &&
            (wallpicnum == w->picnum) &&
            (wal->pal == w->pal) &&
            (wal->xpanning == w->xpanning) &&
            (wal->ypanning == w->ypanning) &&
            (wal->xrepeat == w->xrepeat) &&
            (wal->yrepeat == w->yrepeat) &&
            (walloverpicnum == w->overpicnum) &&
            (wal->shade == w->shade) &&
            ((nwallnum == -1) ||
             ((nwallpicnum == w->nwallpicnum) &&
              (wall[nwallnum].xpanning == w->nwallxpanning) &&
              (wall[nwallnum].ypanning == w->nwallypanning) &&
              (wall[nwallnum].cstat == w->nwallcstat))))
    {
        w->controlstate = 1;
        return; // screw you guys I'm going home
    }
    else
    {
        w->invalidid = invalid;
        w->cstat = wal->cstat;
        w->picnum = wallpicnum;
        w->pal = wal->pal;
        w->xpanning = wal->xpanning;
        w->ypanning = wal->ypanning;
        w->xrepeat = wal->xrepeat;
        w->yrepeat = wal->yrepeat;
        w->overpicnum = walloverpicnum;
        w->shade = wal->shade;
        if (nwallnum != -1)
        {
            w->nwallpicnum = nwallpicnum;
            w->nwallxpanning = wall[nwallnum].xpanning;
            w->nwallypanning = wall[nwallnum].ypanning;
            w->nwallcstat = wall[nwallnum].cstat;
        }
    }

    w->underover = underwall = overwall = 0;

    if (wal->cstat & 8)
        xref = 1;
    else
        xref = 0;

    if ((wal->nextsector == -1) || (wal->cstat & 32))
    {
        memcpy(w->wall.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wall.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wall.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        memcpy(&w->wall.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

        curpicnum = wallpicnum;

        polymer_getbuildmaterial(&w->wall.material, curpicnum, wal->pal, wal->shade);

        if (wal->cstat & 4)
            yref = sec->floorz;
        else
            yref = sec->ceilingz;

        if ((wal->cstat & 32) && (wal->nextsector != -1))
        {
            if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                yref = sec->ceilingz;
            else
                yref = nsec->floorz;
        }

        if (wal->ypanning)
        {
            ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
            if (ypancoef < tilesizy[curpicnum])
                ypancoef *= 2;
            ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
        }
        else
            ypancoef = 0;

        i = 0;
        while (i < 4)
        {
            if ((i == 0) || (i == 3))
                dist = xref;
            else
                dist = (xref == 0);

            w->wall.buffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
            w->wall.buffer[(i * 5) + 4] = (-(float)(yref + (w->wall.buffer[(i * 5) + 1] * 16)) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

            if (wal->cstat & 256) w->wall.buffer[(i * 5) + 4] = -w->wall.buffer[(i * 5) + 4];

            i++;
        }

        w->underover |= 1;
    }
    else
    {
        nnwallnum = wall[nwallnum].point2;

        if (((s->floor.buffer[((wallnum - sec->wallptr) * 5) + 1] != ns->floor.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
                (s->floor.buffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->floor.buffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
                ((s->floor.buffer[((wallnum - sec->wallptr) * 5) + 1] <= ns->floor.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
                 (s->floor.buffer[((wal->point2 - sec->wallptr) * 5) + 1] <= ns->floor.buffer[((nwallnum - nsec->wallptr) * 5) + 1])))
            underwall = 1;

        if ((underwall) || (wal->cstat & 16))
        {
            memcpy(w->wall.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wall.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wall.buffer[10], &ns->floor.buffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->wall.buffer[15], &ns->floor.buffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);

            if (wal->cstat & 2)
            {
                curpicnum = nwallpicnum;
                curpal = wall[nwallnum].pal;
                curshade = wall[nwallnum].shade;
                curxpanning = wall[nwallnum].xpanning;
                curypanning = wall[nwallnum].ypanning;
            }
            else
            {
                curpicnum = wallpicnum;
                curpal = wal->pal;
                curshade = wal->shade;
                curxpanning = wal->xpanning;
                curypanning = wal->ypanning;
            }

            polymer_getbuildmaterial(&w->wall.material, curpicnum, curpal, curshade);

            if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                yref = sec->ceilingz;
            else
                yref = nsec->floorz;

            if (curypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                if (ypancoef < tilesizy[curpicnum])
                    ypancoef *= 2;
                ypancoef *= (float)(curypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                if ((i == 0) || (i == 3))
                    dist = xref;
                else
                    dist = (xref == 0);

                w->wall.buffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + curxpanning) / (float)(tilesizx[curpicnum]);
                w->wall.buffer[(i * 5) + 4] = (-(float)(yref + (w->wall.buffer[(i * 5) + 1] * 16)) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if (wal->cstat & 256) w->wall.buffer[(i * 5) + 4] = -w->wall.buffer[(i * 5) + 4];

                i++;
            }

            if (underwall)
            {
                w->underover |= 1;
                if ((sec->floorstat & 1) && (nsec->floorstat & 1))
                    w->underover |= 4;
            }
            memcpy(w->mask.buffer, &w->wall.buffer[15], sizeof(GLfloat) * 5);
            memcpy(&w->mask.buffer[5], &w->wall.buffer[10], sizeof(GLfloat) * 5);
        }
        else
        {
            memcpy(w->mask.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            memcpy(&w->mask.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }

        if (((s->ceil.buffer[((wallnum - sec->wallptr) * 5) + 1] != ns->ceil.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
                (s->ceil.buffer[((wal->point2 - sec->wallptr) * 5) + 1] != ns->ceil.buffer[((nwallnum - nsec->wallptr) * 5) + 1])) &&
                ((s->ceil.buffer[((wallnum - sec->wallptr) * 5) + 1] >= ns->ceil.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
                 (s->ceil.buffer[((wal->point2 - sec->wallptr) * 5) + 1] >= ns->ceil.buffer[((nwallnum - nsec->wallptr) * 5) + 1])))
            overwall = 1;

        if ((overwall) || (wal->cstat & 16))
        {
            if (w->over.buffer == NULL)
                w->over.buffer = calloc(4, sizeof(GLfloat) * 5);

            memcpy(w->over.buffer, &ns->ceil.buffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->over.buffer[5], &ns->ceil.buffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->over.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            memcpy(&w->over.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

            if ((wal->cstat & 16) || (wal->overpicnum == 0))
                curpicnum = wallpicnum;
            else
                curpicnum = wallpicnum;

            polymer_getbuildmaterial(&w->over.material, curpicnum, wal->pal, wal->shade);

            if (wal->cstat & 16)
            {
                // mask
                polymer_getbuildmaterial(&w->mask.material, wal->overpicnum, wal->pal, wal->shade);
                if (wal->cstat & 128)
                {
                    if (wal->cstat & 512)
                        w->mask.material.diffusemodulation[3] = 0.33f;
                    else
                        w->mask.material.diffusemodulation[3] = 0.66f;
                }
            }

            if (wal->cstat & 4)
                yref = sec->ceilingz;
            else
                yref = nsec->ceilingz;

            if (wal->ypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                if (ypancoef < tilesizy[curpicnum])
                    ypancoef *= 2;
                ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
            }
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                if ((i == 0) || (i == 3))
                    dist = xref;
                else
                    dist = (xref == 0);

                w->over.buffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
                w->over.buffer[(i * 5) + 4] = (-(float)(yref + (w->over.buffer[(i * 5) + 1] * 16)) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if (wal->cstat & 256) w->over.buffer[(i * 5) + 4] = -w->over.buffer[(i * 5) + 4];

                i++;
            }

            if (overwall)
            {
                w->underover |= 2;
                if ((sec->ceilingstat & 1) && (nsec->ceilingstat & 1))
                    w->underover |= 8;
            }
            memcpy(&w->mask.buffer[10], &w->over.buffer[5], sizeof(GLfloat) * 5);
            memcpy(&w->mask.buffer[15], &w->over.buffer[0], sizeof(GLfloat) * 5);

            if (wal->cstat & 16)
            {
                // mask wall pass
                if (wal->cstat & 4)
                    yref = min(sec->floorz, nsec->floorz);
                else
                    yref = max(sec->ceilingz, nsec->ceilingz);

                curpicnum = wal->overpicnum;

                if (wal->ypanning)
                {
                    ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                    if (ypancoef < tilesizy[curpicnum])
                        ypancoef *= 2;
                    ypancoef *= (float)(wal->ypanning) / (256.0f * (float)(tilesizy[curpicnum]));
                }
                else
                    ypancoef = 0;

                i = 0;
                while (i < 4)
                {
                    if ((i == 0) || (i == 3))
                        dist = xref;
                    else
                        dist = (xref == 0);

                    w->mask.buffer[(i * 5) + 3] = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesizx[curpicnum]);
                    w->mask.buffer[(i * 5) + 4] = (-(float)(yref + (w->mask.buffer[(i * 5) + 1] * 16)) / ((tilesizy[curpicnum] * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                    if (wal->cstat & 256) w->mask.buffer[(i * 5) + 4] = -w->mask.buffer[(i * 5) + 4];

                    i++;
                }
            }
        }
        else
        {
            memcpy(&w->mask.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            memcpy(&w->mask.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }
    }

    if ((wal->nextsector == -1) || (wal->cstat & 32))
        memcpy(w->mask.buffer, w->wall.buffer, sizeof(GLfloat) * 4 * 5);

    memcpy(w->bigportal, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[3], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[6], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->bigportal[9], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

    memcpy(&w->cap[0], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->cap[3], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->cap[6], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    memcpy(&w->cap[9], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    w->cap[7] += 1048576; // this number is the result of 1048574 + 2
    w->cap[10] += 1048576; // this one is arbitrary

    polymer_buffertoplane(w->bigportal, NULL, 4, w->wall.plane);
    memcpy(w->over.plane, w->wall.plane, sizeof(w->wall.plane));
    memcpy(w->mask.plane, w->wall.plane, sizeof(w->wall.plane));

    if ((pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->wall.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->wall.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->over.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->over.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->mask.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->mask.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 3, w->bigportal);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 3, 4 * sizeof(GLfloat) * 3, w->cap);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    w->controlstate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated wall %i.\n", wallnum);
}

static void         polymer_drawwall(int16_t sectnum, int16_t wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing wall %i...\n", wallnum);

    w = prwalls[wallnum];

    if ((w->underover & 1) && !(w->underover & 4))
    {
        polymer_drawplane(sectnum, wallnum, &w->wall, 0);
    }

    if ((w->underover & 2) && !(w->underover & 8))
    {
        polymer_drawplane(sectnum, wallnum, &w->over, 0);
    }

    if ((sector[sectnum].ceilingstat & 1) &&
            ((wall[wallnum].nextsector == -1) ||
             !(sector[wall[wallnum].nextsector].ceilingstat & 1)))
    {
        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        if (pr_vbos)
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
            bglVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)(4 * sizeof(GLfloat) * 3));
        }
        else
            bglVertexPointer(3, GL_FLOAT, 0, w->cap);

        bglDrawArrays(GL_QUADS, 0, 4);

        if (pr_vbos)
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

        bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing wall %i...\n", wallnum);
}

#define INDICE(n) ((indices) ? (indices[i+n]*5) : ((i+n)*3))

// HSR
static void         polymer_buffertoplane(GLfloat* buffer, GLushort* indices, int32_t indicecount, GLdouble* plane)
{
    GLfloat         vec1[3], vec2[3];
    int32_t         i;

    i = 0;
    do
    {
        vec1[0] = buffer[(INDICE(1)) + 0] - buffer[(INDICE(0)) + 0];
        vec1[1] = buffer[(INDICE(1)) + 1] - buffer[(INDICE(0)) + 1];
        vec1[2] = buffer[(INDICE(1)) + 2] - buffer[(INDICE(0)) + 2];

        vec2[0] = buffer[(INDICE(2)) + 0] - buffer[(INDICE(1)) + 0];
        vec2[1] = buffer[(INDICE(2)) + 1] - buffer[(INDICE(1)) + 1];
        vec2[2] = buffer[(INDICE(2)) + 2] - buffer[(INDICE(1)) + 2];

        polymer_crossproduct(vec2, vec1, plane);

        // normalize
        vec1[0] = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];
        i+= 3;
    }
    while ((i < indicecount) && (vec1[0] == 0));

    vec1[0] = sqrt(vec1[0]);
    plane[0] /= vec1[0];
    plane[1] /= vec1[0];
    plane[2] /= vec1[0];

    plane[3] = -(plane[0] * buffer[0] + plane[1] * buffer[1] + plane[2] * buffer[2]);
}

static void         polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLdouble* out)
{
    out[0] = in_a[1] * in_b[2] - in_a[2] * in_b[1];
    out[1] = in_a[2] * in_b[0] - in_a[0] * in_b[2];
    out[2] = in_a[0] * in_b[1] - in_a[1] * in_b[0];
}

static void         polymer_pokesector(int16_t sectnum)
{
    sectortype      *sec;
    _prsector       *s;
    walltype        *wal;
    int32_t         i;

    sec = &sector[sectnum];
    s = prsectors[sectnum];
    wal = &wall[sec->wallptr];

    if (!s->controlstate)
        polymer_updatesector(sectnum);

    i = 0;
    while (i < sec->wallnum)
    {
        if ((wal->nextsector != -1) && (!prsectors[wal->nextsector]->controlstate))
            polymer_updatesector(wal->nextsector);
        if (!prwalls[sec->wallptr + i]->controlstate)
            polymer_updatewall(sec->wallptr + i);

        i++;
        wal = &wall[sec->wallptr + i];
    }
}

static void         polymer_extractfrustum(GLfloat* modelview, GLfloat* projection, float* frustum)
{
    GLfloat         matrix[16];
    int32_t         i;

    bglMatrixMode(GL_TEXTURE);
    bglLoadMatrixf(projection);
    bglMultMatrixf(modelview);
    bglGetFloatv(GL_TEXTURE_MATRIX, matrix);
    bglLoadIdentity();
    bglMatrixMode(GL_MODELVIEW);

    i = 0;
    while (i < 4)
    {
        frustum[i] = matrix[(4 * i) + 3] + matrix[4 * i];               // left
        frustum[i + 4] = matrix[(4 * i) + 3] - matrix[4 * i];           // right
        frustum[i + 8] = matrix[(4 * i) + 3] - matrix[(4 * i) + 1];     // top
        frustum[i + 12] = matrix[(4 * i) + 3] + matrix[(4 * i) + 1];    // bottom
        frustum[i + 16] = matrix[(4 * i) + 3] + matrix[(4 * i) + 2];    // near
        i++;
    }
    i = 0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Frustum extracted.\n");
}

static int32_t      polymer_portalinfrustum(int16_t wallnum, float* frustum)
{
    int32_t         i, j, k;
    float           sqdist;
    _prwall         *w;

    w = prwalls[wallnum];

    i = 0;
    while (i < 4)
    {
        j = k = 0;
        while (j < 4)
        {
            sqdist = frustum[(i * 4) + 0] * w->bigportal[(j * 3) + 0] +
                     frustum[(i * 4) + 1] * w->bigportal[(j * 3) + 1] +
                     frustum[(i * 4) + 2] * w->bigportal[(j * 3) + 2] +
                     frustum[(i * 4) + 3];
            if (sqdist < 0)
                k++;
            j++;
        }
        if (k == 4)
            return (0); // OUT !
        i++;
    }

    return (1);
}

static void         polymer_scansprites(int16_t sectnum, spritetype* localtsprite, int32_t* localspritesortcnt)
{
    int32_t         i;
    spritetype      *spr;

    for (i = headspritesect[sectnum];i >=0;i = nextspritesect[i])
    {
        spr = &sprite[i];
        if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                (*localspritesortcnt < MAXSPRITESONSCREEN))
        {
            copybufbyte(spr,&localtsprite[*localspritesortcnt],sizeof(spritetype));
            localtsprite[(*localspritesortcnt)++].owner = i;
        }
    }
}

// SKIES
static void         polymer_getsky(void)
{
    int32_t         i;

    i = 0;
    while (i < numsectors)
    {
        if (sector[i].ceilingstat & 1)
        {
            cursky = sector[i].ceilingpicnum;
            return;
        }
        i++;
    }
}

static void         polymer_drawsky(int16_t tilenum)
{
    pthtyp*         pth;

    drawingskybox = 1;
    pth = gltexcache(tilenum,0,0);
    drawingskybox = 0;

    if (pth && (pth->flags & 4))
        polymer_drawskybox(tilenum);
    else
        polymer_drawartsky(tilenum);
}

static void         polymer_initartsky(void)
{
    GLfloat         halfsqrt2 = 0.70710678f;

    artskydata[0] = -1.0f;          artskydata[1] = 0.0f;           // 0
    artskydata[2] = -halfsqrt2;     artskydata[3] = halfsqrt2;      // 1
    artskydata[4] = 0.0f;           artskydata[5] = 1.0f;           // 2
    artskydata[6] = halfsqrt2;      artskydata[7] = halfsqrt2;      // 3
    artskydata[8] = 1.0f;           artskydata[9] = 0.0f;           // 4
    artskydata[10] = halfsqrt2;     artskydata[11] = -halfsqrt2;    // 5
    artskydata[12] = 0.0f;          artskydata[13] = -1.0f;         // 6
    artskydata[14] = -halfsqrt2;    artskydata[15] = -halfsqrt2;    // 7
}

static void         polymer_drawartsky(int16_t tilenum)
{
    pthtyp*         pth;
    GLuint          glpics[5];
    int32_t         i, j;
    GLfloat         height = 2.45f / 2.0f;

    i = 0;
    while (i < 5)
    {
        if (!waloff[tilenum + i])
            loadtile(tilenum + i);
        pth = gltexcache(tilenum + i, 0, 0);
        glpics[i] = pth ? pth->glpic : 0;
        i++;
    }

    i = 0;
    j = (1<<pskybits);
    while (i < j)
    {
        bglBindTexture(GL_TEXTURE_2D, glpics[pskyoff[i]]);
        polymer_drawartskyquad(i, (i + 1) & (j - 1), height);
        i++;
    }
}

static void         polymer_drawartskyquad(int32_t p1, int32_t p2, GLfloat height)
{
    bglBegin(GL_QUADS);
    bglTexCoord2f(0.0f, 0.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p1 * 2) + 1], height, skybox[p1 * 2]);
    bglVertex3f(artskydata[(p1 * 2) + 1], height, artskydata[p1 * 2]);
    bglTexCoord2f(0.0f, 1.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p1 * 2) + 1], -height, skybox[p1 * 2]);
    bglVertex3f(artskydata[(p1 * 2) + 1], -height, artskydata[p1 * 2]);
    bglTexCoord2f(1.0f, 1.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p2 * 2) + 1], -height, skybox[p2 * 2]);
    bglVertex3f(artskydata[(p2 * 2) + 1], -height, artskydata[p2 * 2]);
    bglTexCoord2f(1.0f, 0.0f);
    //OSD_Printf("PR: drawing %f %f %f\n", skybox[(p2 * 2) + 1], height, skybox[p2 * 2]);
    bglVertex3f(artskydata[(p2 * 2) + 1], height, artskydata[p2 * 2]);
    bglEnd();
}

static void         polymer_drawskybox(int16_t tilenum)
{
    pthtyp*         pth;
    int32_t         i;

    if ((pr_vbos > 0) && (skyboxdatavbo == 0))
    {
        bglGenBuffersARB(1, &skyboxdatavbo);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, skyboxdatavbo);
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5 * 6, skyboxdata, modelvbousage);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    if (pr_vbos > 0)
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, skyboxdatavbo);

    i = 0;
    while (i < 6)
    {
        drawingskybox = i + 1;
        pth = gltexcache(tilenum, 0, 4);

        bglBindTexture(GL_TEXTURE_2D, pth ? pth->glpic : 0);
        if (pr_vbos > 0)
        {
            bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), (GLfloat*)(4 * 5 * i * sizeof(GLfloat)));
            bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), (GLfloat*)(((4 * 5 * i) + 3) * sizeof(GLfloat)));
        } else {
            bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), &skyboxdata[4 * 5 * i]);
            bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &skyboxdata[3 + (4 * 5 * i)]);
        }
        bglDrawArrays(GL_QUADS, 0, 4);

        i++;
    }
    drawingskybox = 0;

    if (pr_vbos > 0)
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    return;
}

// MDSPRITES
static void         polymer_drawmdsprite(spritetype *tspr)
{
    md3model_t*     m;
    mdskinmap_t*    sk;
    md3xyzn_t       *v0, *v1;
    md3surf_t       *s;
    char            lpal;
    float           spos[3];
    float           ang;
    float           scale;
    int32_t         surfi;
    GLfloat*        color;
    int32_t         materialbits;

    m = (md3model_t*)models[tile2model[Ptile2tile(tspr->picnum,sprite[tspr->owner].pal)].modelid];
    updateanimation((md2model_t *)m,tspr);

    lpal = (tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal;

    if ((pr_vbos > 1) && (m->indices == NULL))
        polymer_loadmodelvbos(m);

    spos[0] = tspr->y;
    spos[1] = -(float)(tspr->z) / 16.0f;
    spos[2] = -tspr->x;
    ang = (float)((tspr->ang+spriteext[tspr->owner].angoff) & 2047) / (2048.0f / 360.0f);
    ang -= 90.0f;
    if (((tspr->cstat>>4) & 3) == 2)
        ang -= 90.0f;

    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    scale = (1.0/4.0);
    scale *= m->scale;
    scale *= m->bscale;

    bglTranslatef(spos[0], spos[1], spos[2]);
    bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
    if (((tspr->cstat>>4) & 3) == 2)
    {
        bglTranslatef(0.0f, 0.0, -(float)(tilesizy[tspr->picnum] * tspr->yrepeat) / 8.0f);
        bglRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    }
    else
        bglRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    if ((tspr->cstat & 128) && (((tspr->cstat>>4) & 3) != 2))
        bglTranslatef(0.0f, 0.0, -(float)(tilesizy[tspr->picnum] * tspr->yrepeat) / 8.0f);

    if (tspr->cstat & 8)
    {
        bglTranslatef(0.0f, 0.0, (float)(tilesizy[tspr->picnum] * tspr->yrepeat) / 4.0f);
        bglScalef(1.0f, 1.0f, -1.0f);
    }

    if (tspr->cstat & 4)
        bglScalef(1.0f, -1.0f, 1.0f);

    bglScalef(scale * tspr->xrepeat, scale * tspr->xrepeat, scale * tspr->yrepeat);
    bglTranslatef(0.0f, 0.0, m->zadd * 64);

    polymer_getscratchmaterial(&mdspritematerial);

    color = mdspritematerial.diffusemodulation;

    color[0] = color[1] = color[2] =
        ((float)(numpalookups-min(max((tspr->shade*shadescale)+m->shadeoff,0),numpalookups)))/((float)numpalookups);

    if (!(hictinting[tspr->pal].f&4))
    {
        if (!(m->flags&1) || (!(tspr->owner >= MAXSPRITES) && sector[sprite[tspr->owner].sectnum].floorpal!=0))
        {
            color[0] *= (float)hictinting[tspr->pal].r / 255.0;
            color[1] *= (float)hictinting[tspr->pal].g / 255.0;
            color[2] *= (float)hictinting[tspr->pal].b / 255.0;
            if (hictinting[MAXPALOOKUPS-1].r != 255 || hictinting[MAXPALOOKUPS-1].g != 255 || hictinting[MAXPALOOKUPS-1].b != 255)
            {
                color[0] *= (float)hictinting[MAXPALOOKUPS-1].r / 255.0;
                color[1] *= (float)hictinting[MAXPALOOKUPS-1].g / 255.0;
                color[2] *= (float)hictinting[MAXPALOOKUPS-1].b / 255.0;
            }
        }
        else globalnoeffect=1; //mdloadskin reads this
    }

    if (tspr->cstat & 2)
    {
        if (!(tspr->cstat&512))
            color[3] = 0.66;
        else
            color[3] = 0.33;
    } else
        color[3] = 1.0;

    if (pr_gpusmoothing)
        mdspritematerial.frameprogress = m->interpol;

    for (surfi=0;surfi<m->head.numsurfs;surfi++)
    {
        s = &m->head.surfs[surfi];
        v0 = &s->xyzn[m->cframe*s->numverts];
        v1 = &s->xyzn[m->nframe*s->numverts];

        mdspritematerial.diffusemap =
                mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,sprite[tspr->owner].pal)].skinnum,tspr->pal,surfi);
        if (!mdspritematerial.diffusemap)
            continue;

        if (r_detailmapping && !(tspr->cstat&1024))
        {
            mdspritematerial.detailmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,DETAILPAL,surfi);

            for (sk = m->skinmap; sk; sk = sk->next)
                if ((int32_t)sk->palette == DETAILPAL &&
                    sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                    sk->surfnum == surfi)
                    mdspritematerial.detailscale[0] = mdspritematerial.detailscale[1] = sk->param;
        }

        if (r_glowmapping && !(tspr->cstat&1024))
        {
            mdspritematerial.glowmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,GLOWPAL,surfi);
        }

        if (pr_vbos > 1)
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->texcoords[surfi]);
            bglTexCoordPointer(2, GL_FLOAT, 0, 0);

            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->geometry[surfi]);
            bglVertexPointer(3, GL_SHORT, sizeof(md3xyzn_t), (GLfloat*)(m->cframe * s->numverts * sizeof(md3xyzn_t)));

            if (pr_gpusmoothing)
            {
                mdspritematerial.nextframedata = (GLfloat*)(m->nframe * s->numverts * sizeof(md3xyzn_t));
                mdspritematerial.nextframedatastride = sizeof(md3xyzn_t);
            }

            materialbits = polymer_bindmaterial(mdspritematerial);

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m->indices[surfi]);
            bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, 0);

            polymer_unbindmaterial(materialbits);

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
        else
        {
            bglVertexPointer(3, GL_SHORT, sizeof(md3xyzn_t), v0);
            bglTexCoordPointer(2, GL_FLOAT, 0, s->uv);

            if (pr_gpusmoothing)
            {
                mdspritematerial.nextframedata = (GLfloat*)(v1);
                mdspritematerial.nextframedatastride = sizeof(md3xyzn_t);
            }

            materialbits = polymer_bindmaterial(mdspritematerial);

            bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, s->tris);

            polymer_unbindmaterial(materialbits);
        }
    }

    bglPopMatrix();

    globalnoeffect=0;
}

static void         polymer_loadmodelvbos(md3model_t* m)
{
    int32_t         i;
    md3surf_t       *s;

    m->indices = calloc(m->head.numsurfs, sizeof(GLuint));
    m->texcoords = calloc(m->head.numsurfs, sizeof(GLuint));
    m->geometry = calloc(m->head.numsurfs, sizeof(GLuint));

    bglGenBuffersARB(m->head.numsurfs, m->indices);
    bglGenBuffersARB(m->head.numsurfs, m->texcoords);
    bglGenBuffersARB(m->head.numsurfs, m->geometry);

    i = 0;
    while (i < m->head.numsurfs)
    {
        s = &m->head.surfs[i];

        bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m->indices[i]);
        bglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, s->numtris * sizeof(md3tri_t), s->tris, modelvbousage);

        bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->texcoords[i]);
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, s->numverts * sizeof(md3uv_t), s->uv, modelvbousage);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->geometry[i]);
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, s->numframes * s->numverts * sizeof(md3xyzn_t), s->xyzn, modelvbousage);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        i++;
    }
}

// MATERIALS
static void         polymer_getscratchmaterial(_prmaterial* material)
{
    // this function returns a material that won't validate any bits
    // make sure to keep it up to date with the validation logic in bindmaterial

    // PR_BIT_ANIM_INTERPOLATION
    material->frameprogress = 0.0f;
    material->nextframedata = NULL;
    material->nextframedatastride = 0;
    // PR_BIT_DIFFUSE_MAP
    material->diffusemap = 0;
    material->diffusescale[0] = material->diffusescale[1] = 1.0f;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    material->detailmap = 0;
    material->detailscale[0] = material->detailscale[1] = 1.0f;
    // PR_BIT_DIFFUSE_MODULATION
    material->diffusemodulation[0] =
            material->diffusemodulation[1] =
            material->diffusemodulation[2] =
            material->diffusemodulation[3] = 1.0f;
    // PR_BIT_DIFFUSE_GLOW_MAP
    material->glowmap = 0;
}

static void         polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade)
{
    pthtyp*         pth;
    pthtyp*         detailpth;
    pthtyp*         glowpth;

    polymer_getscratchmaterial(material);

    // PR_BIT_DIFFUSE_MAP
    if (!waloff[tilenum])
        loadtile(tilenum);

    pth = NULL;
    pth = gltexcache(tilenum, pal, 0);

    if (pth)
        material->diffusemap = pth->glpic;

    if (pth->hicr)
    {
        material->diffusescale[0] = pth->hicr->xscale;
        material->diffusescale[1] = pth->hicr->yscale;
    }

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (r_detailmapping && hicfindsubst(tilenum, DETAILPAL, 0))
    {
        detailpth = NULL;
        detailpth = gltexcache(tilenum, DETAILPAL, 0);

        if (detailpth && detailpth->hicr && (detailpth->hicr->palnum == DETAILPAL))
        {
            material->detailmap = detailpth->glpic;

            material->detailscale[0] = detailpth->hicr->xscale;
            material->detailscale[1] = detailpth->hicr->yscale;

            // scale by the diffuse map scale if there's one defined
            if (pth->hicr)
            {
                material->detailscale[0] *= material->diffusescale[0];
                material->detailscale[1] *= material->diffusescale[1];
            }
        }
    }

    // PR_BIT_DIFFUSE_MODULATION
    material->diffusemodulation[0] =
            material->diffusemodulation[1] =
            material->diffusemodulation[2] =
            ((float)(numpalookups-min(max(shade*shadescale,0),numpalookups)))/((float)numpalookups);

    if (pth && (pth->flags & 2) && (pth->palnum != pal))
    {
        material->diffusemodulation[0] *= (float)hictinting[pal].r / 255.0;
        material->diffusemodulation[1] *= (float)hictinting[pal].g / 255.0;
        material->diffusemodulation[2] *= (float)hictinting[pal].b / 255.0;
    }

    // PR_BIT_DIFFUSE_GLOW_MAP
    if (r_fullbrights && pth && pth->flags & 16)
        material->glowmap = pth->ofb->glpic;

    if (r_glowmapping && hicfindsubst(tilenum, GLOWPAL, 0))
    {
        glowpth = NULL;
        glowpth = gltexcache(tilenum, GLOWPAL, 0);

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == GLOWPAL))
            material->glowmap = glowpth->glpic;
    }
}

static int32_t      polymer_bindmaterial(_prmaterial material)
{
    int32_t         programbits;
    int32_t         texunit;

    programbits = prprogrambits[PR_BIT_HEADER].bit;
    programbits |= prprogrambits[PR_BIT_FOOTER].bit;

    // --------- bit validation

    // PR_BIT_*_COMPAT
    if (glinfo.sm4)
        programbits |= prprogrambits[PR_BIT_G8X_COMPAT].bit;
    else
        programbits |= prprogrambits[PR_BIT_NV4X_COMPAT].bit;

    // PR_BIT_ANIM_INTERPOLATION
    if (material.nextframedata)
        programbits |= prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit;

    // PR_BIT_DIFFUSE_MAP
    if (material.diffusemap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_MAP].bit;

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (material.detailmap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit;

    // PR_BIT_DIFFUSE_MODULATION
    if ((material.diffusemodulation[0] != 1.0f) || (material.diffusemodulation[1] != 1.0f) ||
        (material.diffusemodulation[2] != 1.0f) || (material.diffusemodulation[3] != 1.0f))
        programbits |= prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit;

    // PR_BIT_POINT_LIGHT
    programbits |= prprogrambits[PR_BIT_POINT_LIGHT].bit;

    // PR_BIT_DIFFUSE_GLOW_MAP
    if (material.glowmap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_GLOW_MAP].bit;

    // --------- program compiling
    if (!prprograms[programbits].handle)
        polymer_compileprogram(programbits);

    bglUseProgramObjectARB(prprograms[programbits].handle);

    // --------- bit setup

    texunit = 0;

    // PR_BIT_ANIM_INTERPOLATION
    if (programbits & prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit)
    {
        bglEnableVertexAttribArrayARB(prprograms[programbits].attrib_nextFrameData);
        bglVertexAttribPointerARB(prprograms[programbits].attrib_nextFrameData,
                                  3, GL_SHORT, GL_FALSE,
                                  material.nextframedatastride,
                                  material.nextframedata);

        bglUniform1fARB(prprograms[programbits].uniform_frameProgress, material.frameprogress);
    }

    // PR_BIT_DIFFUSE_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.diffusemap);

        bglUniform1iARB(prprograms[programbits].uniform_diffuseMap, texunit);
        bglUniform2fvARB(prprograms[programbits].uniform_diffuseScale, 1, material.diffusescale);

        texunit++;
    }

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.detailmap);

        bglUniform1iARB(prprograms[programbits].uniform_detailMap, texunit);
        bglUniform2fvARB(prprograms[programbits].uniform_detailScale, 1, material.detailscale);

        texunit++;
    }

    // PR_BIT_DIFFUSE_MODULATION
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit)
    {
        bglColor4f(material.diffusemodulation[0],
                   material.diffusemodulation[1],
                   material.diffusemodulation[2],
                   material.diffusemodulation[3]);
    }

    // PR_BIT_POINT_LIGHT
    if (programbits & prprogrambits[PR_BIT_POINT_LIGHT].bit)
    {
        int lightCount;
        float range[4];
        float color[8];
        float pos[6];
        float lightpos[8];

        lightCount = 2;

        pos[0] = 62208;
        pos[1] = 42000 / 16.0;
        pos[2] = -6919;

        lightpos[0] = pos[0] * rootmodelviewmatrix[0] +
                      pos[1] * rootmodelviewmatrix[4] +
                      pos[2] * rootmodelviewmatrix[8] +
                             + rootmodelviewmatrix[12];
        lightpos[1] = pos[0] * rootmodelviewmatrix[1] +
                      pos[1] * rootmodelviewmatrix[5] +
                      pos[2] * rootmodelviewmatrix[9] +
                             + rootmodelviewmatrix[13];
        lightpos[2] = pos[0] * rootmodelviewmatrix[2] +
                      pos[1] * rootmodelviewmatrix[6] +
                      pos[2] * rootmodelviewmatrix[10] +
                             + rootmodelviewmatrix[14];

        color[0] = 0.1f;
        color[1] = 0.1f;
        color[2] = 1.0f;

        range[0] = 1024.0f / 1000.0;
        range[1] = 2048.0f / 1000.0;

        pos[3] = globalposy;
        pos[4] = -globalposz / 16.0;
        pos[5] = -globalposx;

        lightpos[4] = pos[3] * rootmodelviewmatrix[0] +
                      pos[4] * rootmodelviewmatrix[4] +
                      pos[5] * rootmodelviewmatrix[8] +
                             + rootmodelviewmatrix[12];
        lightpos[5] = pos[3] * rootmodelviewmatrix[1] +
                      pos[4] * rootmodelviewmatrix[5] +
                      pos[5] * rootmodelviewmatrix[9] +
                             + rootmodelviewmatrix[13];
        lightpos[6] = pos[3] * rootmodelviewmatrix[2] +
                      pos[4] * rootmodelviewmatrix[6] +
                      pos[5] * rootmodelviewmatrix[10] +
                             + rootmodelviewmatrix[14];

        color[4] = 0.5f;
        color[5] = 0.5f;
        color[6] = 0.5f;

        range[2] = 0.0f / 1000.0;
        range[3] = 2048.0f / 1000.0;

        bglLightfv(GL_LIGHT0, GL_AMBIENT, lightpos);
        bglLightfv(GL_LIGHT0, GL_DIFFUSE, color);
        bglLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, &range[0]);
        bglLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, &range[1]);

        bglLightfv(GL_LIGHT1, GL_AMBIENT, &lightpos[4]);
        bglLightfv(GL_LIGHT1, GL_DIFFUSE, &color[4]);
        bglLightfv(GL_LIGHT1, GL_CONSTANT_ATTENUATION, &range[2]);
        bglLightfv(GL_LIGHT1, GL_LINEAR_ATTENUATION, &range[3]);

        range[3] = 0;

        bglLightfv(GL_LIGHT2, GL_LINEAR_ATTENUATION, &range[3]);
        bglLightfv(GL_LIGHT3, GL_LINEAR_ATTENUATION, &range[3]);
        bglLightfv(GL_LIGHT4, GL_LINEAR_ATTENUATION, &range[3]);
        bglLightfv(GL_LIGHT5, GL_LINEAR_ATTENUATION, &range[3]);
        bglLightfv(GL_LIGHT6, GL_LINEAR_ATTENUATION, &range[3]);
        bglLightfv(GL_LIGHT7, GL_LINEAR_ATTENUATION, &range[3]);

        if (glinfo.sm4)
            bglUniform1iARB(prprograms[programbits].uniform_lightCount, lightCount);
    }

    // PR_BIT_DIFFUSE_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_GLOW_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.glowmap);

        bglUniform1iARB(prprograms[programbits].uniform_glowMap, texunit);

        texunit++;
    }

    bglActiveTextureARB(GL_TEXTURE0_ARB);

    return (programbits);
}

static void         polymer_unbindmaterial(int32_t programbits)
{
    // PR_BIT_ANIM_INTERPOLATION
    if (programbits & prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit)
    {
        bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_nextFrameData);
    }

    bglUseProgramObjectARB(0);
}

static void         polymer_compileprogram(int32_t programbits)
{
    int32_t         i, enabledbits;
    GLhandleARB     vert, frag, program;
    GLcharARB*      source[PR_BIT_COUNT * 2];
    GLcharARB       infobuffer[PR_INFO_LOG_BUFFER_SIZE];

    // --------- VERTEX
    vert = bglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

    enabledbits = i = 0;
    while (i < PR_BIT_COUNT)
    {
        if (programbits & prprogrambits[i].bit)
            source[enabledbits++] = prprogrambits[i].vert_def;
        i++;
    }
    i = 0;
    while (i < PR_BIT_COUNT)
    {
        if (programbits & prprogrambits[i].bit)
            source[enabledbits++] = prprogrambits[i].vert_prog;
        i++;
    }

    bglShaderSourceARB(vert, enabledbits, (const GLcharARB**)source, NULL);

    bglCompileShaderARB(vert);

    // --------- FRAGMENT
    frag = bglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    enabledbits = i = 0;
    while (i < PR_BIT_COUNT)
    {
        if (programbits & prprogrambits[i].bit)
            source[enabledbits++] = prprogrambits[i].frag_def;
        i++;
    }
    i = 0;
    while (i < PR_BIT_COUNT)
    {
        if (programbits & prprogrambits[i].bit)
            source[enabledbits++] = prprogrambits[i].frag_prog;
        i++;
    }

    bglShaderSourceARB(frag, enabledbits, (const GLcharARB**)source, NULL);

    bglCompileShaderARB(frag);

    // --------- PROGRAM
    program = bglCreateProgramObjectARB();

    bglAttachObjectARB(program, vert);
    bglAttachObjectARB(program, frag);

    bglLinkProgramARB(program);

    bglGetInfoLogARB(program, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);

    prprograms[programbits].handle = program;

    if (pr_verbosity >= 1) OSD_Printf("Compiling GPU program with bits %i...\n", programbits);
    if (infobuffer[0]) {
        if (pr_verbosity >= 1) OSD_Printf("Info log:\n%s\n", infobuffer);
        bglGetShaderSourceARB(vert, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);
        if (pr_verbosity >= 1) OSD_Printf("Vertex source dump:\n%s\n", infobuffer);
        bglGetShaderSourceARB(frag, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);
        if (pr_verbosity >= 1) OSD_Printf("Shader source dump:\n%s\n", infobuffer);
    }

    // --------- ATTRIBUTE/UNIFORM LOCATIONS

    // PR_BIT_ANIM_INTERPOLATION
    if (programbits & prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit)
    {
        prprograms[programbits].attrib_nextFrameData = bglGetAttribLocationARB(program, "nextFrameData");
        prprograms[programbits].uniform_frameProgress = bglGetUniformLocationARB(program, "frameProgress");
    }

    // PR_BIT_DIFFUSE_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MAP].bit)
    {
        prprograms[programbits].uniform_diffuseMap = bglGetUniformLocationARB(program, "diffuseMap");
        prprograms[programbits].uniform_diffuseScale = bglGetUniformLocationARB(program, "diffuseScale");
    }

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit)
    {
        prprograms[programbits].uniform_detailMap = bglGetUniformLocationARB(program, "detailMap");
        prprograms[programbits].uniform_detailScale = bglGetUniformLocationARB(program, "detailScale");
    }

    // PR_BIT_POINT_LIGHT
    if (programbits & prprogrambits[PR_BIT_POINT_LIGHT].bit && glinfo.sm4)
    {
        prprograms[programbits].uniform_lightCount = bglGetUniformLocationARB(program, "lightCount");
    }

    // PR_BIT_DIFFUSE_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_GLOW_MAP].bit)
    {
        prprograms[programbits].uniform_glowMap = bglGetUniformLocationARB(program, "glowMap");
    }
}

#endif
