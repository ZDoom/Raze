// blah
#ifdef POLYMOST

#define POLYMER_C
#include "polymer.h"
#include "engine_priv.h"

// CVARS
int32_t         pr_lighting = 1;
int32_t         pr_normalmapping = 1;
int32_t         pr_specularmapping = 1;
int32_t         pr_shadows = 1;
int32_t         pr_shadowcount = 5;
int32_t         pr_shadowdetail = 4;
int32_t         pr_maxlightpasses = 5;
int32_t         pr_maxlightpriority = PR_MAXLIGHTPRIORITY;
int32_t         pr_fov = 426;           // appears to be the classic setting.
int32_t         pr_billboardingmode = 1;
int32_t         pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int32_t         pr_wireframe = 0;
int32_t         pr_vbos = 2;
int32_t         pr_gpusmoothing = 1;
int32_t         pr_overrideparallax = 0;
float           pr_parallaxscale = 0.1f;
float           pr_parallaxbias = 0.0f;
int32_t         pr_overridespecular = 0;
float           pr_specularpower = 15.0f;
float           pr_specularfactor = 1.0f;

GLenum          mapvbousage = GL_STREAM_DRAW_ARB;
GLenum          modelvbousage = GL_STATIC_DRAW_ARB;

// BUILD DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];

_prplane        spriteplane;
_prmaterial     mdspritematerial;

static GLfloat  vertsprite[4 * 5] =
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

static GLfloat  horizsprite[4 * 5] =
{
    -0.5f, 0.0f, 0.5f,
    0.0f, 0.0f,
    0.5f, 0.0f, 0.5f,
    1.0f, 0.0f,
    0.5f, 0.0f, -0.5f,
    1.0f, 1.0f,
    -0.5f, 0.0f, -0.5f,
    0.0f, 1.0f,
};

static GLfloat  skyboxdata[4 * 5 * 6] =
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
int32_t         curlight;

_prlight        staticlights[PR_MAXLIGHTS];
int32_t         staticlightcount;

_prlight        gamelights[PR_MAXLIGHTS];
int32_t         gamelightcount;

_prlight        framelights[PR_MAXLIGHTS];
int32_t         framelightcount;

static GLfloat  shadowBias[] =
{
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
};

// MATERIALS
_prprogrambit   prprogrambits[PR_BIT_COUNT] = {
    {
        1 << PR_BIT_HEADER,
        // vert_def
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "\n",
        // vert_prog
        "",
        // frag_def
        "#version 120\n"
        "#extension GL_ARB_texture_rectangle : enable\n"
        "\n",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_ANIM_INTERPOLATION,
        // vert_def
        "attribute vec4 nextFrameData;\n"
        "attribute vec4 nextFrameNormal;\n"
        "uniform float frameProgress;\n"
        "\n",
        // vert_prog
        "  vec4 currentFramePosition;\n"
        "  vec4 nextFramePosition;\n"
        "\n"
        "  currentFramePosition = curVertex * (1.0 - frameProgress);\n"
        "  nextFramePosition = nextFrameData * frameProgress;\n"
        "  curVertex = currentFramePosition + nextFramePosition;\n"
        "\n"
        "  currentFramePosition = vec4(curNormal, 1.0) * (1.0 - frameProgress);\n"
        "  nextFramePosition = nextFrameNormal * frameProgress;\n"
        "  curNormal = vec3(currentFramePosition + nextFramePosition);\n"
        "\n",
        // frag_def
        "",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_LIGHTING_PASS,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "",
        // frag_prog
        "  isLightingPass = 1;\n"
        "  result = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "\n",
    },
    {
        1 << PR_BIT_NORMAL_MAP,
        // vert_def
        "attribute vec3 T;\n"
        "attribute vec3 B;\n"
        "attribute vec3 N;\n"
        "uniform vec3 eyePosition;\n"
        "varying vec3 tangentSpaceEyeVec;\n"
        "\n",
        // vert_prog
        "  TBN = transpose(mat3(T, B, N));\n"
        "  tangentSpaceEyeVec = eyePosition - vec3(curVertex);\n"
        "  tangentSpaceEyeVec = TBN * tangentSpaceEyeVec;\n"
        "\n"
        "  isNormalMapped = 1;\n"
        "\n",
        // frag_def
        "uniform sampler2D normalMap;\n"
        "uniform vec2 normalBias;\n"
        "varying vec3 tangentSpaceEyeVec;\n"
        "\n",
        // frag_prog
        "  vec4 normalStep;\n"
        "  float biasedHeight;\n"
        "\n"
        "  eyeVec = normalize(tangentSpaceEyeVec);\n"
        "\n"
        "  for (int i = 0; i < 4; i++) {\n"
        "    normalStep = texture2D(normalMap, commonTexCoord.st);\n"
        "    biasedHeight = normalStep.a * normalBias.x - normalBias.y;\n"
        "    commonTexCoord += (biasedHeight - commonTexCoord.z) * normalStep.z * eyeVec;\n"
        "  }\n"
        "\n"
        "  normalTexel = texture2D(normalMap, commonTexCoord.st);\n"
        "\n"
        "  isNormalMapped = 1;\n"
        "\n",
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
        "  diffuseTexel = texture2D(diffuseMap, commonTexCoord.st);\n"
        "  if (isLightingPass == 0)\n"
        "    result *= diffuseTexel;\n"
        "\n",
    },
    {
        1 << PR_BIT_DIFFUSE_DETAIL_MAP,
        // vert_def
        "uniform vec2 detailScale;\n"
        "varying vec2 fragDetailScale;\n"
        "\n",
        // vert_prog
        "  fragDetailScale = detailScale;\n"
        "  if (isNormalMapped == 0)\n"
        "    gl_TexCoord[1] = vec4(detailScale, 1.0, 1.0) * gl_MultiTexCoord0;\n"
        "\n",
        // frag_def
        "uniform sampler2D detailMap;\n"
        "varying vec2 fragDetailScale;\n"
        "\n",
        // frag_prog
        "  if (isNormalMapped == 0)\n"
        "    result *= texture2D(detailMap, gl_TexCoord[1].st);\n"
        "  else\n"
        "    result *= texture2D(detailMap, commonTexCoord.st * fragDetailScale);\n"
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
        "  if (isLightingPass == 0)\n"
        "    result *= vec4(gl_Color);\n"
        "\n",
    },
    {
        1 << PR_BIT_SPECULAR_MAP,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform sampler2D specMap;\n"
        "\n",
        // frag_prog
        "  specTexel = texture2D(specMap, commonTexCoord.st);\n"
        "\n"
        "  isSpecularMapped = 1;\n"
        "\n",
    },
    {
        1 << PR_BIT_SPECULAR_MATERIAL,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform vec2 specMaterial;\n"
        "\n",
        // frag_prog
        "  specularMaterial = specMaterial;\n"
        "\n",
    },
    {
        1 << PR_BIT_MIRROR_MAP,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform sampler2DRect mirrorMap;\n"
        "\n",
        // frag_prog
        "  vec4 mirrorTexel;\n"
        "  vec2 mirrorCoords;\n"
        "\n"
        "  mirrorCoords = gl_FragCoord.st;\n"
        "  if (isNormalMapped == 1) {\n"
        "    mirrorCoords += 100.0 * (normalTexel.rg - 0.5);\n"
        "  }\n"
        "  mirrorTexel = texture2DRect(mirrorMap, mirrorCoords);\n"
        "  result = vec4((result.rgb * (1.0 - specTexel.a)) + (mirrorTexel.rgb * specTexel.rgb * specTexel.a), result.a);\n"
        "\n",
    },
    {
        1 << PR_BIT_FOG,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "",
        // frag_prog
        "  float fragDepth;\n"
        "  float fogFactor;\n"
        "\n"
        "  fragDepth = gl_FragCoord.z / gl_FragCoord.w / 35.0;\n"
        "  fragDepth *= fragDepth;\n"
        "  fogFactor = exp2(-gl_Fog.density * gl_Fog.density * fragDepth * 1.442695);\n"
        "  result.rgb = mix(gl_Fog.color.rgb, result.rgb, fogFactor);\n"
        "\n",
    },
    {
        1 << PR_BIT_GLOW_MAP,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform sampler2D glowMap;\n"
        "\n",
        // frag_prog
        "  vec4 glowTexel;\n"
        "\n"
        "  glowTexel = texture2D(glowMap, commonTexCoord.st);\n"
        "  result = vec4((result.rgb * (1.0 - glowTexel.a)) + (glowTexel.rgb * glowTexel.a), result.a);\n"
        "\n",
    },
    {
        1 << PR_BIT_SHADOW_MAP,
        // vert_def
        "uniform mat4 shadowProjMatrix;\n"
        "\n",
        // vert_prog
        "  gl_TexCoord[2] = shadowProjMatrix * curVertex;\n"
        "\n",
        // frag_def
        "uniform sampler2DShadow shadowMap;\n"
        "\n",
        // frag_prog
        "  shadowResult = shadow2DProj(shadowMap, gl_TexCoord[2]).a;\n"
        "\n",
    },
    {
        1 << PR_BIT_LIGHT_MAP,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform sampler2D lightMap;\n"
        "\n",
        // frag_prog
        "  lightTexel = texture2D(lightMap, gl_TexCoord[2].st / gl_TexCoord[2].q).rgb;\n"
        "\n",
    },
    {
        1 << PR_BIT_SPOT_LIGHT,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform vec3 spotDir;\n"
        "uniform vec2 spotRadius;\n"
        "\n",
        // frag_prog
        "  spotVector = spotDir;\n"
        "  spotCosRadius = spotRadius;\n"
        "  isSpotLight = 1;\n"
        "\n",
    },
    {
        1 << PR_BIT_POINT_LIGHT,
        // vert_def
        "varying vec3 vertexNormal;\n"
        "varying vec3 eyeVector;\n"
        "varying vec3 lightVector;\n"
        "varying vec3 tangentSpaceLightVector;\n"
        "\n",
        // vert_prog
        "  vec3 vertexPos;\n"
        "\n"
        "  vertexPos = vec3(gl_ModelViewMatrix * curVertex);\n"
        "  eyeVector = -vertexPos;\n"
        "  lightVector = gl_LightSource[0].ambient.rgb - vertexPos;\n"
        "\n"
        "  if (isNormalMapped == 1) {\n"
        "    tangentSpaceLightVector = gl_LightSource[0].specular.rgb - vec3(curVertex);\n"
        "    tangentSpaceLightVector = TBN * tangentSpaceLightVector;\n"
        "  } else\n"
        "    vertexNormal = normalize(gl_NormalMatrix * curNormal);\n"
        "\n",
        // frag_def
        "varying vec3 vertexNormal;\n"
        "varying vec3 eyeVector;\n"
        "varying vec3 lightVector;\n"
        "varying vec3 tangentSpaceLightVector;\n"
        "\n",
        // frag_prog
        "  vec2 lightRange;\n"
        "  float pointLightDistance;\n"
        "  float lightAttenuation;\n"
        "  float spotAttenuation;\n"
        "  vec3 N, L, E, R, D;\n"
        "  vec3 lightDiffuse;\n"
        "  float lightSpecular;\n"
        "  float NdotL;\n"
        "  float spotCosAngle;\n"
        "\n"
        "  L = normalize(lightVector);\n"
        "\n"
        "  pointLightDistance = length(lightVector);\n"
        "  lightRange.x = gl_LightSource[0].constantAttenuation;\n"
        "  lightRange.y = gl_LightSource[0].linearAttenuation;\n"
        "\n"
        "  lightAttenuation = clamp(1.0 - pointLightDistance * lightRange.y, 0.0, 1.0);\n"
        "  spotAttenuation = 1.0;\n"
        "\n"
        "  if (isSpotLight == 1) {\n"
        "    D = normalize(spotVector);\n"
        "    spotCosAngle = dot(-L, D);\n"
        "    spotAttenuation = clamp((spotCosAngle - spotCosRadius.x) * spotCosRadius.y, 0.0, 1.0);\n"
        "  }\n"
        "\n"
        "  if (isNormalMapped == 1) {\n"
        "    E = eyeVec;\n"
        "    N = normalize(2.0 * (normalTexel.rgb - 0.5));\n"
        "    L = normalize(tangentSpaceLightVector);\n"
        "  } else {\n"
        "    E = normalize(eyeVector);\n"
        "    N = normalize(vertexNormal);\n"
        "  }\n"
        "  NdotL = max(dot(N, L), 0.0);\n"
        "\n"
        "  R = reflect(-L, N);\n"
        "\n"
        "  lightDiffuse = gl_Color.a * shadowResult * lightTexel *\n"
        "                 gl_LightSource[0].diffuse.rgb * lightAttenuation * spotAttenuation;\n"
        "  result += vec4(lightDiffuse * diffuseTexel.a * diffuseTexel.rgb * NdotL, 0.0);\n"
        "\n"
        "  if (isSpecularMapped == 0)\n"
        "    specTexel.rgb = diffuseTexel.rgb * diffuseTexel.a;\n"
        "\n"
        "  lightSpecular = pow( max(dot(R, E), 0.0), specularMaterial.x * specTexel.a) * specularMaterial.y;\n"
        "  result += vec4(lightDiffuse * specTexel.rgb * lightSpecular, 0.0);\n"
        "\n",
    },
    {
        1 << PR_BIT_FOOTER,
        // vert_def
        "void main(void)\n"
        "{\n"
        "  vec4 curVertex = gl_Vertex;\n"
        "  vec3 curNormal = gl_Normal;\n"
        "  int isNormalMapped = 0;\n"
        "  mat3 TBN;\n"
        "\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "\n",
        // vert_prog
        "  gl_Position = gl_ModelViewProjectionMatrix * curVertex;\n"
        "}\n",
        // frag_def
        "void main(void)\n"
        "{\n"
        "  vec3 commonTexCoord = vec3(gl_TexCoord[0].st, 0.0);\n"
        "  vec4 result = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "  vec4 diffuseTexel = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "  vec4 specTexel = vec4(1.0, 1.0, 1.0, 1.0);\n"
        "  vec4 normalTexel;\n"
        "  int isLightingPass = 0;\n"
        "  int isNormalMapped = 0;\n"
        "  int isSpecularMapped = 0;\n"
        "  vec3 eyeVec;\n"
        "  int isSpotLight = 0;\n"
        "  vec3 spotVector;\n"
        "  vec2 spotCosRadius;\n"
        "  float shadowResult = 1.0;\n"
        "  vec2 specularMaterial = vec2(15.0, 1.0);\n"
        "  vec3 lightTexel = vec3(1.0, 1.0, 1.0);\n"
        "\n",
        // frag_prog
        "  gl_FragColor = result;\n"
        "}\n",
    }
};

_prprograminfo  prprograms[1 << PR_BIT_COUNT];

int32_t         overridematerial;

// RENDER TARGETS
_prrt           *prrts;

// CONTROL
GLfloat         spritemodelview[16];
GLfloat         rootmodelviewmatrix[16];
GLfloat         *curmodelviewmatrix;
GLfloat         rootskymodelviewmatrix[16];
GLfloat         *curskymodelviewmatrix;

static int16_t  sectorqueue[MAXSECTORS];
static int16_t  querydelay[MAXSECTORS];
static GLuint   queryid[MAXWALLS];
static int16_t  drawingstate[MAXSECTORS];

float           horizang;
int16_t         viewangle;

int32_t         depth;
_prmirror       mirrors[10];

GLUtesselator*  prtess;

int16_t         cursky;

_pranimatespritesinfo asi;

// EXTERNAL FUNCTIONS
int32_t             polymer_init(void)
{
    int32_t         i;

    if (pr_verbosity >= 1) OSD_Printf("Initializing Polymer subsystem...\n");

    Bmemset(&prsectors[0], 0, sizeof(prsectors[0]) * MAXSECTORS);
    Bmemset(&prwalls[0], 0, sizeof(prwalls[0]) * MAXWALLS);

    prtess = bgluNewTess();
    if (prtess == 0)
    {
        OSD_Printf("PR : Tessellation object initialization failed!\n");
        return (0);
    }

    polymer_loadboard();

    polymer_initartsky();
    skyboxdatavbo = 0;

    if (spriteplane.buffer == NULL) {
        spriteplane.buffer = Bcalloc(4, sizeof(GLfloat) * 5);
        spriteplane.vertcount = 4;
    }

    i = 0;
    while (i < nextmodelid)
    {
        if (models[i])
        {
            md3model_t* m;

            m = (md3model_t*)models[i];
            m->indices = NULL;
        }
        i++;
    }

    i = 0;
    while (i < (1 << PR_BIT_COUNT))
    {
        prprograms[i].handle = 0;
        i++;
    }

    overridematerial = 0xFFFFFFFF;

    polymer_initrendertargets(pr_shadowcount + 1);

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete.\n");
    return (1);
}

void                polymer_uninit(void)
{
    polymer_freeboard();
}

void                polymer_glinit(void)
{
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

    bglDisable(GL_BLEND);
    bglDisable(GL_ALPHA_TEST);

    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bgluPerspective((float)(pr_fov) / (2048.0f / 360.0f), (float)xdim / (float)ydim, 0.01f, 100.0f);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglDisable(GL_FOG);

    bglEnable(GL_CULL_FACE);
    bglCullFace(GL_BACK);
}

void                polymer_loadboard(void)
{
    int32_t         i;

    polymer_freeboard();

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
    int32_t         i, cursectflorz, cursectceilz;
    float           skyhoriz, ang, tiltang;
    float           pos[3];
    pthtyp*         pth;

    if (!rendmode) return;

    begindrawing();

    // TODO: support for screen resizing
    // frameoffset = frameplace + windowy1*bytesperline + windowx1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    // fogcalc needs this
    gvisibility = ((float)globalvisibility)*FOGSCALE;

    ang = (float)(daang) / (2048.0f / 360.0f);
    horizang = (float)(-getangle(128, dahoriz-100)) / (2048.0f / 360.0f);
    tiltang = (gtang * 90.0f);

    if (searchit == 2) polymer_editorselect();

    pos[0] = daposy;
    pos[1] = -(float)(daposz) / 16.0f;
    pos[2] = -daposx;

    polymer_resetlights();
    if (pr_lighting)
        polymer_applylights();

    depth = 0;

    if (pr_shadows && lightcount && (pr_shadowcount > 0))
        polymer_prepareshadows();

    // hack for parallax skies
    skyhoriz = horizang;
    if (skyhoriz < -180.0f)
        skyhoriz += 360.0f;

    drawingskybox = 1;
    pth = gltexcache(cursky,0,0);
    drawingskybox = 0;

    // if it's not a skybox, make the sky parallax
    // the angle factor is computed from eyeballed values
    // need to recompute it if we ever change the max horiz amplitude
    if (!pth || !(pth->flags & 4))
        skyhoriz /= 4.3027;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglRotatef(tiltang, 0.0f, 0.0f, -1.0f);
    bglRotatef(skyhoriz, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

    bglScalef(1.0f / 1000.0f, 1.0f / 1000.0f, 1.0f / 1000.0f);
    bglTranslatef(-pos[0], -pos[1], -pos[2]);

    bglGetFloatv(GL_MODELVIEW_MATRIX, rootskymodelviewmatrix);
    curskymodelviewmatrix = rootskymodelviewmatrix;

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglRotatef(tiltang, 0.0f, 0.0f, -1.0f);
    bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);

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
        prsectors[i]->ceil.drawn = 0;
        prsectors[i]->floor.drawn = 0;
        prsectors[i]->wallsproffset = 0.0f;
        prsectors[i]->floorsproffset = 0.0f;
        i++;
    }
    i = 0;
    while (i < numwalls)
    {
        prwalls[i]->controlstate = 0;
        prwalls[i]->wall.drawn = 0;
        prwalls[i]->over.drawn = 0;
        prwalls[i]->mask.drawn = 0;
        i++;
    }

    getzsofslope(dacursectnum, daposx, daposy, &cursectceilz, &cursectflorz);

    // external view (editor)
    if ((dacursectnum < 0) || (dacursectnum >= numsectors) ||
            (daposz > cursectflorz) ||
            (daposz < cursectceilz))
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
        enddrawing();
        return;
    }

    // GO!
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
    enddrawing();
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
    sectortype      *sec;
    walltype        *wal;
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);

    sec = &sector[sectorofwall(maskwall[damaskwallcnt])];
    wal = &wall[maskwall[damaskwallcnt]];
    w = prwalls[maskwall[damaskwallcnt]];

    fogcalc(wal->shade,sec->visibility,sec->floorpal);
    bglFogf(GL_FOG_DENSITY,fogresult);
    bglFogfv(GL_FOG_COLOR,fogcol);

    bglEnable(GL_CULL_FACE);

    polymer_drawplane(&w->mask);

    bglDisable(GL_CULL_FACE);
}

void                polymer_drawsprite(int32_t snum)
{
    int32_t         curpicnum, xsize, ysize, tilexoff, tileyoff, xoff, yoff, i;
    spritetype      *tspr;
    float           xratio, yratio, ang;
    float           spos[3];
    GLfloat         *inbuffer;

    if (pr_verbosity >= 3) OSD_Printf("PR : Sprite %i...\n", snum);

    tspr = tspriteptr[snum];

    if (tspr->owner < 0 || tspr->picnum < 0) return;

    fogcalc(tspr->shade,sector[tspr->sectnum].visibility,sector[tspr->sectnum].floorpal);
    bglFogf(GL_FOG_DENSITY,fogresult);
    bglFogfv(GL_FOG_COLOR,fogcol);

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

    spriteplane.material.diffusemodulation[3] *=  (1.0f - spriteext[tspr->owner].alpha);

    if (tspr->cstat & 16384) spriteplane.material.diffusemodulation[3] = 0.0f;

    if (((tspr->cstat>>4) & 3) == 0)
        xratio = (float)(tspr->xrepeat) * 0.20f; // 32 / 160
    else
        xratio = (float)(tspr->xrepeat) * 0.25f;

    yratio = (float)(tspr->yrepeat) * 0.25f;

    xsize = tilesizx[curpicnum];
    ysize = tilesizy[curpicnum];

    if (usehightile && h_xsize[curpicnum])
    {
        xsize = h_xsize[curpicnum];
        ysize = h_ysize[curpicnum];
    }

    xsize *= xratio;
    ysize *= yratio;

    tilexoff = (int32_t)tspr->xoffset;
    tileyoff = (int32_t)tspr->yoffset;
    tilexoff += (int8_t)((usehightile&&h_xsize[curpicnum])?(h_xoffs[curpicnum]):((picanm[curpicnum]>>8)&255));
    tileyoff += (int8_t)((usehightile&&h_xsize[curpicnum])?(h_yoffs[curpicnum]):((picanm[curpicnum]>>16)&255));

    xoff = tilexoff * xratio;
    yoff = tileyoff * yratio;

    if ((tspr->cstat & 128) && (((tspr->cstat>>4) & 3) != 2))
        yoff -= ysize / 2;

    spos[0] = tspr->y;
    spos[1] = -(float)(tspr->z) / 16.0f;
    spos[2] = -tspr->x;

    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();

    inbuffer = vertsprite;

    if (pr_billboardingmode && !((tspr->cstat>>4) & 3))
    {
        // do surgery on the face tspr to make it look like a wall sprite
        tspr->cstat |= 16;
        tspr->ang = (viewangle + 1024) & 2047;
    }

    switch ((tspr->cstat>>4) & 3)
    {
    case 0:
        ang = (float)((viewangle) & 2047) / (2048.0f / 360.0f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglRotatef(-horizang, 1.0f, 0.0f, 0.0f);
        bglTranslatef((float)(-xoff), (float)(yoff), 0.0f);
        bglScalef((float)(xsize), (float)(ysize), 1.0f);

//         bglPolygonOffset(0.0f, 0.0f);
        break;
    case 1:
        ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglTranslatef((float)(-xoff), (float)(yoff), 0.0f);
        bglScalef((float)(xsize), (float)(ysize), 1.0f);

        prsectors[tspr->sectnum]->wallsproffset += 0.5f;

        if (!depth || mirrors[depth-1].plane)
            bglPolygonOffset(-1.0f, -1.0f);
        break;
    case 2:
        ang = (float)((tspr->ang + 1024) & 2047) / (2048.0f / 360.0f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        if (tspr->cstat & 8) {
            bglRotatef(-180.0, 0.0f, 0.0f, 1.0f);
            spriteplane.material.diffusescale[0] = -spriteplane.material.diffusescale[0];
        }
        bglTranslatef((float)(-xoff), 1.0f, (float)(yoff));
        bglScalef((float)(xsize), 1.0f, (float)(ysize));

        inbuffer = horizsprite;

        prsectors[tspr->sectnum]->floorsproffset += 0.5f;
        if (!depth || mirrors[depth-1].plane)
            bglPolygonOffset(-1.0f, -1.0f);
        break;
    }

    if ((tspr->cstat & 4) && (((tspr->cstat>>4) & 3) != 2))
        spriteplane.material.diffusescale[0] = -spriteplane.material.diffusescale[0];
    if (!(tspr->cstat & 4) && (((tspr->cstat>>4) & 3) == 2))
        spriteplane.material.diffusescale[0] = -spriteplane.material.diffusescale[0];

    if ((tspr->cstat & 8) && (((tspr->cstat>>4) & 3) != 2))
        spriteplane.material.diffusescale[1] = -spriteplane.material.diffusescale[1];

    bglGetFloatv(GL_MODELVIEW_MATRIX, spritemodelview);
    bglPopMatrix();

    Bmemcpy(spriteplane.buffer, inbuffer, sizeof(GLfloat) * 4 * 5);

    i = 0;
    while (i < 4)
    {
        polymer_transformpoint(&inbuffer[i * 5], &spriteplane.buffer[i * 5], spritemodelview);
        i++;
    }

    polymer_computeplane(&spriteplane);

    spriteplane.lightcount = 0;
    i = 0;
    while (i < lightcount)
    {
        if (polymer_planeinlight(&spriteplane, &prlights[i]))
        {
            spriteplane.lights[spriteplane.lightcount] = i;
            spriteplane.lightcount++;
        }
        i++;
    }

    if ((tspr->cstat & 64) && ((tspr->cstat>>4) & 3))
        bglEnable(GL_CULL_FACE);

    if (!depth || mirrors[depth-1].plane)
        bglEnable(GL_POLYGON_OFFSET_FILL);

    polymer_drawplane(&spriteplane);

    if (!depth || mirrors[depth-1].plane)
        bglDisable(GL_POLYGON_OFFSET_FILL);

    if ((tspr->cstat & 64) && ((tspr->cstat>>4) & 3))
        bglDisable(GL_CULL_FACE);

}

void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio)
{
    asi.animatesprites = animatesprites;
    asi.x = x;
    asi.y = y;
    asi.a = a;
    asi.smoothratio = smoothratio;
}

// CORE
static void         polymer_displayrooms(int16_t dacursectnum)
{
    sectortype      *sec;
    int32_t         i;
    GLint           result;
    int16_t         doquery;
    int32_t         front;
    int32_t         back;
    GLfloat         localskymodelviewmatrix[16];
    GLfloat         localmodelviewmatrix[16];
    GLfloat         localprojectionmatrix[16];
    float           frustum[5 * 4];
    int32_t         localspritesortcnt;
    spritetype      localtsprite[MAXSPRITESONSCREEN];
    int16_t         localmaskwall[MAXWALLSB];
    int16_t         localmaskwallcnt;
    _prmirror       mirrorlist[10];
    int             mirrorcount;
    int32_t         gx, gy, gz, px, py, pz;
    GLdouble        plane[4];
    float           coeff;

    curmodelviewmatrix = localmodelviewmatrix;
    bglGetFloatv(GL_MODELVIEW_MATRIX, localmodelviewmatrix);
    bglGetFloatv(GL_PROJECTION_MATRIX, localprojectionmatrix);

    polymer_extractfrustum(localmodelviewmatrix, localprojectionmatrix, frustum);

    Bmemset(querydelay, 0, sizeof(int16_t) * numsectors);
    Bmemset(queryid, 0, sizeof(GLuint) * numwalls);
    Bmemset(drawingstate, 0, sizeof(int16_t) * numsectors);

    front = 0;
    back = 1;
    sectorqueue[0] = dacursectnum;
    drawingstate[dacursectnum] = 1;

    localspritesortcnt = localmaskwallcnt = 0;

    mirrorcount = 0;

    bglDisable(GL_DEPTH_TEST);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    polymer_drawsky(cursky);
    bglEnable(GL_DEPTH_TEST);

    // depth-only occlusion testing pass
//     overridematerial = 0;

    while (front != back)
    {
        sec = &sector[sectorqueue[front]];

        polymer_pokesector(sectorqueue[front]);
        polymer_drawsector(sectorqueue[front]);
        polymer_scansprites(sectorqueue[front], localtsprite, &localspritesortcnt);

        doquery = 0;

        i = 0;
        while (i < sec->wallnum)
        {
            polymer_drawwall(sectorqueue[front], sec->wallptr + i);

            // if we have a level boundary somewhere in the sector,
            // consider these walls as visportals
            if (wall[sec->wallptr + i].nextsector == -1)
                doquery = 1;

            i++;
        }

        i = 0;
        while (i < sec->wallnum)
        {
            if ((wall[sec->wallptr + i].nextsector != -1) &&
                (wallvisible(sec->wallptr + i)) &&
                (polymer_planeinfrustum(&prwalls[sec->wallptr + i]->mask, frustum)))
            {
                if ((prwalls[sec->wallptr + i]->mask.vertcount == 4) &&
                    !(prwalls[sec->wallptr + i]->underover & 4) &&
                    !(prwalls[sec->wallptr + i]->underover & 8))
                {
                    // early exit for closed sectors
                    _prwall         *w;

                    w = prwalls[sec->wallptr + i];

                    if ((w->mask.buffer[(0 * 5) + 1] >= w->mask.buffer[(3 * 5) + 1]) &&
                        (w->mask.buffer[(1 * 5) + 1] >= w->mask.buffer[(2 * 5) + 1]))
                    {
                        i++;
                        continue;
                    }
                }

                if (wall[sec->wallptr + i].cstat & 48)
                    localmaskwall[localmaskwallcnt++] = sec->wallptr + i;

                if (!depth && (overridematerial & prprogrambits[PR_BIT_MIRROR_MAP].bit) &&
                     wall[sec->wallptr + i].overpicnum == 560 &&
                     wall[sec->wallptr + i].cstat & 32)
                {
                    mirrorlist[mirrorcount].plane = &prwalls[sec->wallptr + i]->mask;
                    mirrorlist[mirrorcount].sectnum = sectorqueue[front];
                    mirrorlist[mirrorcount].wallnum = sec->wallptr + i;
                    mirrorcount++;
                }

                if (!(wall[sec->wallptr + i].cstat & 32)) {
                    if (doquery && (!drawingstate[wall[sec->wallptr + i].nextsector]))
                    {
                        float pos[3], sqdist;
                        int32_t oldoverridematerial;

                        pos[0] = globalposy;
                        pos[1] = -(float)(globalposz) / 16.0f;
                        pos[2] = -globalposx;

                        sqdist = prwalls[sec->wallptr + i]->mask.plane[0] * pos[0] +
                                 prwalls[sec->wallptr + i]->mask.plane[1] * pos[1] +
                                 prwalls[sec->wallptr + i]->mask.plane[2] * pos[2] +
                                 prwalls[sec->wallptr + i]->mask.plane[3];

                        // hack to avoid occlusion querying portals that are too close to the viewpoint
                        // this is needed because of the near z-clipping plane;
                        if (sqdist < 100)
                            queryid[sec->wallptr + i] = 0xFFFFFFFF;
                        else {
                            _prwall         *w;

                            w = prwalls[sec->wallptr + i];

                            bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                            bglDepthMask(GL_FALSE);

                            bglGenQueriesARB(1, &queryid[sec->wallptr + i]);
                            bglBeginQueryARB(GL_SAMPLES_PASSED_ARB, queryid[sec->wallptr + i]);

                            oldoverridematerial = overridematerial;
                            overridematerial = 0;

                            if ((w->underover & 4) && (w->underover & 1))
                                polymer_drawplane(&w->wall);
                            polymer_drawplane(&w->mask);
                            if ((w->underover & 8) && (w->underover & 2))
                                polymer_drawplane(&w->over);

                            overridematerial = oldoverridematerial;

                            bglEndQueryARB(GL_SAMPLES_PASSED_ARB);

                            bglDepthMask(GL_TRUE);
                            bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        }
                    } else
                        queryid[sec->wallptr + i] = 1;
                }
            }

            i++;
        }

        i = 0;
        while (i < sec->wallnum)
        {
            if ((queryid[sec->wallptr + i]) &&
                (!drawingstate[wall[sec->wallptr + i].nextsector]))
            {
                // REAP
                result = 0;
                if (doquery && (queryid[sec->wallptr + i] != 0xFFFFFFFF))
                {
                    bglGetQueryObjectivARB(queryid[sec->wallptr + i],
                                           GL_QUERY_RESULT_ARB,
                                           &result);
                    bglDeleteQueriesARB(1, &queryid[sec->wallptr + i]);
                } else if (queryid[sec->wallptr + i] == 0xFFFFFFFF)
                    result = 1;

                queryid[sec->wallptr + i] = 0;

                if (result || !doquery)
                {
                    sectorqueue[back++] = wall[sec->wallptr + i].nextsector;
                    drawingstate[wall[sec->wallptr + i].nextsector] = 1;
                }
            }

            i++;
        }

        front++;
    }

    // do the actual shaded drawing
//     overridematerial = 0xFFFFFFFF;

    // go through the sector queue again
//     front = 0;
//     while (front < back)
//     {
//         sec = &sector[sectorqueue[front]];
// 
//         polymer_drawsector(sectorqueue[front]);
// 
//         i = 0;
//         while (i < sec->wallnum)
//         {
//             polymer_drawwall(sectorqueue[front], sec->wallptr + i);
// 
//             i++;
//         }
// 
//         front++;
//     }

    i = 0;
    while (i < mirrorcount)
    {
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, prrts[0].fbo);
        bglPushAttrib(GL_VIEWPORT_BIT);
        bglViewport(0, 0, xdim, ydim);

        bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Bmemcpy(localskymodelviewmatrix, curskymodelviewmatrix, sizeof(GLfloat) * 16);
        curskymodelviewmatrix = localskymodelviewmatrix;

        bglMatrixMode(GL_MODELVIEW);
        bglPushMatrix();

        plane[0] = mirrorlist[i].plane->plane[0];
        plane[1] = mirrorlist[i].plane->plane[1];
        plane[2] = mirrorlist[i].plane->plane[2];
        plane[3] = mirrorlist[i].plane->plane[3];

        bglClipPlane(GL_CLIP_PLANE0, plane);
        polymer_inb4mirror(mirrorlist[i].plane->buffer, mirrorlist[i].plane->plane);
        bglCullFace(GL_FRONT);
        //bglEnable(GL_CLIP_PLANE0);

        if (mirrorlist[i].wallnum >= 0)
            preparemirror(globalposx, globalposy, 0, globalang, 0,
                          mirrorlist[i].wallnum, 0, &gx, &gy, &viewangle);

        gx = globalposx;
        gy = globalposy;
        gz = globalposz;

        // map the player pos from build to polymer
        px = globalposy;
        py = -globalposz / 16;
        pz = -globalposx;

        // calculate new player position on the other side of the mirror
        // this way the basic build visibility shit can be used (wallvisible)
        coeff = mirrorlist[i].plane->plane[0] * px +
                mirrorlist[i].plane->plane[1] * py +
                mirrorlist[i].plane->plane[2] * pz +
                mirrorlist[i].plane->plane[3];

        coeff /= (float)(mirrorlist[i].plane->plane[0] * mirrorlist[i].plane->plane[0] +
                         mirrorlist[i].plane->plane[1] * mirrorlist[i].plane->plane[1] +
                         mirrorlist[i].plane->plane[2] * mirrorlist[i].plane->plane[2]);

        px = -coeff*mirrorlist[i].plane->plane[0]*2 + px;
        py = -coeff*mirrorlist[i].plane->plane[1]*2 + py;
        pz = -coeff*mirrorlist[i].plane->plane[2]*2 + pz;

        // map back from polymer to build
        globalposx = -pz;
        globalposy = px;
        globalposz = -py * 16;

        mirrors[depth++] = mirrorlist[i];
        polymer_displayrooms(mirrorlist[i].sectnum);
        depth--;

        globalposx = gx;
        globalposy = gy;
        globalposz = gz;

        bglDisable(GL_CLIP_PLANE0);
        bglCullFace(GL_BACK);
        bglMatrixMode(GL_MODELVIEW);
        bglPopMatrix();

        bglPopAttrib();
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        mirrorlist[i].plane->material.mirrormap = prrts[0].color;
        polymer_drawplane(mirrorlist[i].plane);
        mirrorlist[i].plane->material.mirrormap = 0;

        i++;
    }

    spritesortcnt = localspritesortcnt;
    Bmemcpy(tsprite, localtsprite, sizeof(spritetype) * spritesortcnt);
    maskwallcnt = localmaskwallcnt;
    Bmemcpy(maskwall, localmaskwall, sizeof(int16_t) * maskwallcnt);

    if (depth)
    {
        // drawmasks needs these
        cosglobalang = sintable[(viewangle+512)&2047];
        singlobalang = sintable[viewangle&2047];
        cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
        sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);

        if (mirrors[depth - 1].plane)
            display_mirror = 1;
        polymer_animatesprites();
        if (mirrors[depth - 1].plane)
            display_mirror = 0;

        bglDisable(GL_CULL_FACE);
        drawmasks();
        bglEnable(GL_CULL_FACE);
    }

    return;
}

static void         polymer_drawplane(_prplane* plane)
{
    int32_t         materialbits;

    // debug code for drawing plane inverse TBN
//     bglDisable(GL_TEXTURE_2D);
//     bglBegin(GL_LINES);
//     bglColor4f(1.0, 0.0, 0.0, 1.0);
//     bglVertex3f(plane->buffer[0],
//                 plane->buffer[1],
//                 plane->buffer[2]);
//     bglVertex3f(plane->buffer[0] + plane->t[0] * 50,
//                 plane->buffer[1] + plane->t[1] * 50,
//                 plane->buffer[2] + plane->t[2] * 50);
//     bglColor4f(0.0, 1.0, 0.0, 1.0);
//     bglVertex3f(plane->buffer[0],
//                 plane->buffer[1],
//                 plane->buffer[2]);
//     bglVertex3f(plane->buffer[0] + plane->b[0] * 50,
//                 plane->buffer[1] + plane->b[1] * 50,
//                 plane->buffer[2] + plane->b[2] * 50);
//     bglColor4f(0.0, 0.0, 1.0, 1.0);
//     bglVertex3f(plane->buffer[0],
//                 plane->buffer[1],
//                 plane->buffer[2]);
//     bglVertex3f(plane->buffer[0] + plane->n[0] * 50,
//                 plane->buffer[1] + plane->n[1] * 50,
//                 plane->buffer[2] + plane->n[2] * 50);
//     bglEnd();
//     bglEnable(GL_TEXTURE_2D);

    // debug code for drawing plane normals
//     bglDisable(GL_TEXTURE_2D);
//     bglBegin(GL_LINES);
//     bglColor4f(1.0, 1.0, 1.0, 1.0);
//     bglVertex3f(plane->buffer[0],
//                 plane->buffer[1],
//                 plane->buffer[2]);
//     bglVertex3f(plane->buffer[0] + plane->plane[0] * 50,
//                 plane->buffer[1] + plane->plane[1] * 50,
//                 plane->buffer[2] + plane->plane[2] * 50);
//     bglEnd();
//     bglEnable(GL_TEXTURE_2D);

    bglNormal3f((float)(plane->plane[0]), (float)(plane->plane[1]), (float)(plane->plane[2]));

    if (plane->vbo && (pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, plane->vbo);
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), NULL);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), (GLfloat*)(3 * sizeof(GLfloat)));
        if (plane->indices)
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, plane->ivbo);
    } else {
        bglVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), plane->buffer);
        bglTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), &plane->buffer[3]);
    }

    curlight = 0;

    while ((curlight == 0) || ((curlight < plane->lightcount) && (curlight < pr_maxlightpasses)))
    {
        materialbits = polymer_bindmaterial(plane->material, plane->lights, plane->lightcount);

        if (materialbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
        {
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_T, plane->t);
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_B, plane->b);
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_N, plane->plane);
        }

        if (plane->indices)
        {
            if (plane->vbo && (pr_vbos > 0))
                bglDrawElements(GL_TRIANGLES, plane->indicescount, GL_UNSIGNED_SHORT, NULL);
            else
                bglDrawElements(GL_TRIANGLES, plane->indicescount, GL_UNSIGNED_SHORT, plane->indices);
        } else
            bglDrawArrays(GL_QUADS, 0, 4);

        polymer_unbindmaterial(materialbits);

        curlight++;
    }

    if (plane->vbo && (pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        if (plane->indices)
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }

    plane->drawn = 1;
}

static inline void  polymer_inb4mirror(GLfloat* buffer, GLfloat* plane)
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

    bglPushMatrix();
    bglLoadMatrixf(curskymodelviewmatrix);
    bglMultMatrixf(reflectionmatrix);
    bglGetFloatv(GL_MODELVIEW_MATRIX, curskymodelviewmatrix);
    bglPopMatrix();
}

static void         polymer_animatesprites(void)
{
    if (asi.animatesprites)
        asi.animatesprites(globalposx, globalposy, viewangle, asi.smoothratio);
}

static void         polymer_freeboard(void)
{
    int32_t         i;

    i = 0;
    while (i < MAXSECTORS)
    {
        if (prsectors[i])
        {
            if (prsectors[i]->verts) Bfree(prsectors[i]->verts);
            if (prsectors[i]->floor.buffer) Bfree(prsectors[i]->floor.buffer);
            if (prsectors[i]->ceil.buffer) Bfree(prsectors[i]->ceil.buffer);
            if (prsectors[i]->floor.indices) Bfree(prsectors[i]->floor.indices);
            if (prsectors[i]->ceil.indices) Bfree(prsectors[i]->ceil.indices);
            if (prsectors[i]->ceil.vbo) bglDeleteBuffersARB(1, &prsectors[i]->ceil.vbo);
            if (prsectors[i]->ceil.ivbo) bglDeleteBuffersARB(1, &prsectors[i]->ceil.ivbo);
            if (prsectors[i]->floor.vbo) bglDeleteBuffersARB(1, &prsectors[i]->floor.vbo);
            if (prsectors[i]->floor.ivbo) bglDeleteBuffersARB(1, &prsectors[i]->floor.ivbo);

            Bfree(prsectors[i]);
            prsectors[i] = NULL;
        }

        i++;
    }

    i = 0;
    while (i < MAXWALLS)
    {
        if (prwalls[i])
        {
            if (prwalls[i]->bigportal) Bfree(prwalls[i]->bigportal);
            if (prwalls[i]->mask.buffer) Bfree(prwalls[i]->mask.buffer);
            if (prwalls[i]->cap) Bfree(prwalls[i]->cap);
            if (prwalls[i]->wall.buffer) Bfree(prwalls[i]->wall.buffer);
            if (prwalls[i]->wall.vbo) bglDeleteBuffersARB(1, &prwalls[i]->wall.vbo);
            if (prwalls[i]->over.vbo) bglDeleteBuffersARB(1, &prwalls[i]->over.vbo);
            if (prwalls[i]->mask.vbo) bglDeleteBuffersARB(1, &prwalls[i]->mask.vbo);
            if (prwalls[i]->stuffvbo) bglDeleteBuffersARB(1, &prwalls[i]->stuffvbo);

            Bfree(prwalls[i]);
            prwalls[i] = NULL;
        }

        i++;
    }
}

static void         polymer_editorselect(void)
{
        int32_t     i, n;
        double      ox, oy, oz, ox2, oy2, oz2, px[6], py[6], pz[6];

        //Polymost supports true look up/down :) Here, we convert horizon to angle.
        //gchang&gshang are cos&sin of this angle (respectively)
        ghalfx = (double)halfxdimen; grhalfxdown10 = 1.0/(((double)ghalfx)*1024);
        ghoriz = (double)globalhoriz;

        gvisibility = ((float)globalvisibility)*FOGSCALE;

        ghoriz = (double)(ydimen>>1);

        //global cos/sin tilt angle
        gctang = cos(gtang);
        gstang = sin(gtang);
        if (fabs(gstang) < .001) //This hack avoids nasty precision bugs in domost()
        { gstang = 0; if (gctang > 0) gctang = 1.0; else gctang = -1.0; }

        //Generate viewport trapezoid (for handling screen up/down)
        px[0] = px[3] = 0-1; px[1] = px[2] = windowx2+1-windowx1+2;
        py[0] = py[1] = 0-1; py[2] = py[3] = windowy2+1-windowy1+2; n = 4;
        for (i=0; i<n; i++)
        {
            ox = px[i]-ghalfx; oy = py[i]-ghoriz; oz = ghalfx;

            //Tilt rotation (backwards)
            ox2 = ox*gctang + oy*gstang;
            oy2 = oy*gctang - ox*gstang;
            oz2 = oz;

            //Up/down rotation (backwards)
            px[i] = ox2;
            py[i] = oy2*gchang + oz2*gshang;
            pz[i] = oz2*gchang - oy2*gshang;
        }

        if (searchit == 2)
        {
            int32_t vx, vy, vz;
            int32_t cz, fz;
            hitdata_t hitinfo;
            vec3_t vect;

            ox2 = searchx-ghalfx; oy2 = searchy-ghoriz; oz2 = ghalfx;

            //Tilt rotation
            ox = ox2*gctang + oy2*gstang;
            oy = oy2*gctang - ox2*gstang;
            oz = oz2;

            //Up/down rotation
            ox2 = oz*gchang - oy*gshang;
            oy2 = ox;
            oz2 = oy*gchang + oz*gshang;

            //Standard Left/right rotation
            vx = (int32_t)(ox2*((float)cosglobalang) - oy2*((float)singlobalang));
            vy = (int32_t)(ox2*((float)singlobalang) + oy2*((float)cosglobalang));
            vz = (int32_t)(oz2*16384.0);

            vect.x = globalposx;
            vect.y = globalposy;
            vect.z = globalposz;

            hitallsprites = 1;
            hitscan((const vec3_t *)&vect,globalcursectnum, //Start position
                vx>>12,vy>>12,vz>>8,&hitinfo,0xffff0030);
            getzsofslope(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
            hitallsprites = 0;

            searchsector = hitinfo.hitsect;
            if (hitinfo.pos.z<cz) searchstat = 1; else if (hitinfo.pos.z>fz) searchstat = 2; else if (hitinfo.hitwall >= 0)
            {
                searchwall = hitinfo.hitwall; searchstat = 0;
                if (wall[hitinfo.hitwall].nextwall >= 0)
                {
                    int32_t cz, fz;
                    getzsofslope(wall[hitinfo.hitwall].nextsector,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
                    if (hitinfo.pos.z > fz)
                    {
                        if (wall[hitinfo.hitwall].cstat&2) //'2' bottoms of walls
                            searchwall = wall[hitinfo.hitwall].nextwall;
                    }
                    else if ((hitinfo.pos.z > cz) && (wall[hitinfo.hitwall].cstat&(16+32))) //masking or 1-way
                        searchstat = 4;
                }
            }
            else if (hitinfo.hitsprite >= 0) { searchwall = hitinfo.hitsprite; searchstat = 3; }
            else
            {
                int32_t cz, fz;
                getzsofslope(hitinfo.hitsect,hitinfo.pos.x,hitinfo.pos.y,&cz,&fz);
                if ((hitinfo.pos.z<<1) < cz+fz) searchstat = 1; else searchstat = 2;
                //if (vz < 0) searchstat = 1; else searchstat = 2; //Won't work for slopes :/
            }
            searchit = 0;
        }
}

// SECTORS
static int32_t      polymer_initsector(int16_t sectnum)
{
    sectortype      *sec;
    _prsector*      s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing sector %i...\n", sectnum);

    sec = &sector[sectnum];
    s = Bcalloc(1, sizeof(_prsector));
    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize sector %i : Bmalloc failed.\n", sectnum);
        return (0);
    }

    s->verts = Bcalloc(sec->wallnum, sizeof(GLdouble) * 3);
    s->floor.buffer = Bcalloc(sec->wallnum, sizeof(GLfloat) * 5);
    s->floor.vertcount = sec->wallnum;
    s->ceil.buffer = Bcalloc(sec->wallnum, sizeof(GLfloat) * 5);
    s->ceil.vertcount = sec->wallnum;
    if ((s->verts == NULL) || (s->floor.buffer == NULL) || (s->ceil.buffer == NULL))
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize geometry of sector %i : Bmalloc failed.\n", sectnum);
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
            needfloor ||
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

            // relative texturing
            if (curstat & 64)
            {
                xpancoef = wal->x - wall[sec->wallptr].x;
                ypancoef = wall[sec->wallptr].y - wal->y;

                tex = xpancoef * secangsin + ypancoef * secangcos;
                tey = xpancoef * secangcos - ypancoef * secangsin;
            } else {
                tex = wal->x;
                tey = -wal->y;
            }

            if ((curstat & (2+64)) == (2+64))
            {
                heidiff = curbuffer[(i*5)+1] - curbuffer[1];
                // don't forget the sign, tey could be negative with concave sectors
                if (tey >= 0)
                    tey = sqrt((tey * tey) + (heidiff * heidiff));
                else
                    tey = -sqrt((tey * tey) + (heidiff * heidiff));
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
        polymer_computeplane(&s->floor);
        polymer_computeplane(&s->ceil);
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
        s->floor.indices = Brealloc(s->floor.indices, s->indicescount * sizeof(GLushort));
        s->ceil.indices = Brealloc(s->ceil.indices, s->indicescount * sizeof(GLushort));
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
        s->floor.indices = Bcalloc(s->indicescount, sizeof(GLushort));
        s->ceil.indices = Bcalloc(s->indicescount, sizeof(GLushort));
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
    s->floor.indicescount = s->ceil.indicescount = s->indicescount;

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

    fogcalc(sec->floorshade,sec->visibility,sec->floorpal);
    bglFogf(GL_FOG_DENSITY,fogresult);
    bglFogfv(GL_FOG_COLOR,fogcol);

    if (!(sec->floorstat & 1))
        polymer_drawplane(&s->floor);

    fogcalc(sec->ceilingshade,sec->visibility,sec->ceilingpal);
    bglFogf(GL_FOG_DENSITY,fogresult);
    bglFogfv(GL_FOG_COLOR,fogcol);

    if (!(sec->ceilingstat & 1))
        polymer_drawplane(&s->ceil);

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

// WALLS
static int32_t      polymer_initwall(int16_t wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initalizing wall %i...\n", wallnum);

    w = Bcalloc(1, sizeof(_prwall));
    if (w == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Cannot initialize wall %i : Bmalloc failed.\n", wallnum);
        return (0);
    }

    if (w->mask.buffer == NULL) {
        w->mask.buffer = Bcalloc(4, sizeof(GLfloat) * 5);
        w->mask.vertcount = 4;
    }
    if (w->bigportal == NULL)
        w->bigportal = Bcalloc(4, sizeof(GLfloat) * 5);
    if (w->cap == NULL)
        w->cap = Bcalloc(4, sizeof(GLfloat) * 3);

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
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 8 * sizeof(GLfloat) * 5, NULL, mapvbousage);

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
    if (sec->wallptr > wallnum)
        return; // the map is horribly corrupt
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

    if (w->wall.buffer == NULL) {
        w->wall.buffer = Bcalloc(4, sizeof(GLfloat) * 5);
        w->wall.vertcount = 4;
    }

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

    if (wal->nextsector == -1)
    {
        Bmemcpy(w->wall.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

        if (wal->nextsector == -1)
            curpicnum = wallpicnum;
        else
            curpicnum = walloverpicnum;

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

        if ((s->floor.buffer[((wallnum - sec->wallptr) * 5) + 1] < ns->floor.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
            (s->floor.buffer[((wal->point2 - sec->wallptr) * 5) + 1] < ns->floor.buffer[((nwallnum - nsec->wallptr) * 5) + 1]))
            underwall = 1;

        if ((underwall) || (wal->cstat & 16) || (wal->cstat & 32))
        {
            if (s->floor.buffer[((wallnum - sec->wallptr) * 5) + 1] < ns->floor.buffer[((nnwallnum - nsec->wallptr) * 5) + 1])
                Bmemcpy(w->wall.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            else
                Bmemcpy(w->wall.buffer, &ns->floor.buffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[10], &ns->floor.buffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[15], &ns->floor.buffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);

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
            Bmemcpy(w->mask.buffer, &w->wall.buffer[15], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[5], &w->wall.buffer[10], sizeof(GLfloat) * 5);
        }
        else
        {
            Bmemcpy(w->mask.buffer, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }

        if ((s->ceil.buffer[((wallnum - sec->wallptr) * 5) + 1] > ns->ceil.buffer[((nnwallnum - nsec->wallptr) * 5) + 1]) ||
            (s->ceil.buffer[((wal->point2 - sec->wallptr) * 5) + 1] > ns->ceil.buffer[((nwallnum - nsec->wallptr) * 5) + 1]))
            overwall = 1;

        if ((overwall) || (wal->cstat & 16) || (wal->cstat & 32))
        {
            if (w->over.buffer == NULL) {
                w->over.buffer = Bcalloc(4, sizeof(GLfloat) * 5);
                w->over.vertcount = 4;
            }

            Bmemcpy(w->over.buffer, &ns->ceil.buffer[(nnwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            Bmemcpy(&w->over.buffer[5], &ns->ceil.buffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            if (s->ceil.buffer[((wal->point2 - sec->wallptr) * 5) + 1] > ns->ceil.buffer[((nwallnum - nsec->wallptr) * 5) + 1])
                Bmemcpy(&w->over.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
            else
                Bmemcpy(&w->over.buffer[10], &ns->ceil.buffer[(nwallnum - nsec->wallptr) * 5], sizeof(GLfloat) * 3);
            Bmemcpy(&w->over.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

            if ((wal->cstat & 16) || (wal->overpicnum == 0))
                curpicnum = wallpicnum;
            else
                curpicnum = wallpicnum;

            polymer_getbuildmaterial(&w->over.material, curpicnum, wal->pal, wal->shade);

            if ((wal->cstat & 16) || (wal->cstat & 32))
            {
                // mask
                polymer_getbuildmaterial(&w->mask.material, walloverpicnum, wal->pal, wal->shade);
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
            Bmemcpy(&w->mask.buffer[10], &w->over.buffer[5], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[15], &w->over.buffer[0], sizeof(GLfloat) * 5);

            if ((wal->cstat & 16) || (wal->cstat & 32))
            {
                // mask wall pass
                if (wal->cstat & 4)
                    yref = min(sec->floorz, nsec->floorz);
                else
                    yref = max(sec->ceilingz, nsec->ceilingz);

                if (wal->cstat & 32)
                {
                    if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                        yref = sec->ceilingz;
                    else
                        yref = nsec->ceilingz;
                }

                curpicnum = walloverpicnum;

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
            Bmemcpy(&w->mask.buffer[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 5);
        }
    }

    if (wal->nextsector == -1)
        Bmemcpy(w->mask.buffer, w->wall.buffer, sizeof(GLfloat) * 4 * 5);

    Bmemcpy(w->bigportal, &s->floor.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[5], &s->floor.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[10], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[15], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);

    Bmemcpy(&w->cap[0], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->cap[3], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->cap[6], &s->ceil.buffer[(wal->point2 - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    Bmemcpy(&w->cap[9], &s->ceil.buffer[(wallnum - sec->wallptr) * 5], sizeof(GLfloat) * 3);
    w->cap[7] += 1048576; // this number is the result of 1048574 + 2
    w->cap[10] += 1048576; // this one is arbitrary

    if (w->underover & 1)
        polymer_computeplane(&w->wall);
    if (w->underover & 2)
        polymer_computeplane(&w->over);
    polymer_computeplane(&w->mask);

    if ((pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->wall.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->wall.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->over.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->over.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->mask.vbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->mask.buffer);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat) * 5, w->bigportal);
        bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5, 4 * sizeof(GLfloat) * 3, w->cap);
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    w->controlstate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated wall %i.\n", wallnum);
}

static void         polymer_drawwall(int16_t sectnum, int16_t wallnum)
{
    sectortype      *sec;
    walltype        *wal;
    _prwall         *w;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing wall %i...\n", wallnum);

    sec = &sector[sectnum];
    wal = &wall[wallnum];
    w = prwalls[wallnum];

    fogcalc(wal->shade,sec->visibility,sec->floorpal);
    bglFogf(GL_FOG_DENSITY,fogresult);
    bglFogfv(GL_FOG_COLOR,fogcol);

    if ((w->underover & 1) && !(w->underover & 4))
    {
        polymer_drawplane(&w->wall);
    }

    if ((w->underover & 2) && !(w->underover & 8))
    {
        polymer_drawplane(&w->over);
    }

    if ((wall[wallnum].cstat & 32) && (wall[wallnum].nextsector != -1))
        polymer_drawplane(&w->mask);

    if ((sector[sectnum].ceilingstat & 1) &&
            ((wall[wallnum].nextsector == -1) ||
             !(sector[wall[wallnum].nextsector].ceilingstat & 1)))
    {
        bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        if (pr_vbos)
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
            bglVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)(4 * sizeof(GLfloat) * 5));
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

#define INDICE(n) ((p->indices) ? (p->indices[i+n]*5) : ((i+n)*5))

// HSR
static void         polymer_computeplane(_prplane* p)
{
    GLfloat         vec1[5], vec2[5], norm, r;// BxN[3], NxT[3], TxB[3];
    int32_t         i;
    GLfloat*        buffer;
    GLfloat*        t;
    GLfloat*        b;
    GLfloat*        n;
    GLfloat*        plane;

    if (p->indices && (p->indicescount < 3))
        return; // corrupt sector (E3L4, I'm looking at you)

    buffer = p->buffer;
    t = p->t;
    b = p->b;
    n = p->n;
    plane = p->plane;

    i = 0;
    do
    {
        vec1[0] = buffer[(INDICE(1)) + 0] - buffer[(INDICE(0)) + 0]; //x1
        vec1[1] = buffer[(INDICE(1)) + 1] - buffer[(INDICE(0)) + 1]; //y1
        vec1[2] = buffer[(INDICE(1)) + 2] - buffer[(INDICE(0)) + 2]; //z1
        vec1[3] = buffer[(INDICE(1)) + 3] - buffer[(INDICE(0)) + 3]; //s1
        vec1[4] = buffer[(INDICE(1)) + 4] - buffer[(INDICE(0)) + 4]; //t1

        vec2[0] = buffer[(INDICE(2)) + 0] - buffer[(INDICE(1)) + 0]; //x2
        vec2[1] = buffer[(INDICE(2)) + 1] - buffer[(INDICE(1)) + 1]; //y2
        vec2[2] = buffer[(INDICE(2)) + 2] - buffer[(INDICE(1)) + 2]; //z2
        vec2[3] = buffer[(INDICE(2)) + 3] - buffer[(INDICE(1)) + 3]; //s2
        vec2[4] = buffer[(INDICE(2)) + 4] - buffer[(INDICE(1)) + 4]; //t2

        polymer_crossproduct(vec2, vec1, plane);

        norm = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];

        // hack to work around a precision issue with slopes
        if ((norm >= 15000) ||
            (((p->indices) ? p->indices[i+2] : i+2) >= p->vertcount))
        {
            // normalize the normal/plane equation and calculate its plane norm
            norm = -sqrt(norm);
            norm = 1.0 / norm;
            plane[0] *= norm;
            plane[1] *= norm;
            plane[2] *= norm;
            plane[3] = -(plane[0] * buffer[0] + plane[1] * buffer[1] + plane[2] * buffer[2]);

            // calculate T and B
            r = 1.0 / (vec1[3] * vec2[4] - vec2[3] * vec1[4]);

            // tangent
            t[0] = (vec2[4] * vec1[0] - vec1[4] * vec2[0]) * r;
            t[1] = (vec2[4] * vec1[1] - vec1[4] * vec2[1]) * r;
            t[2] = (vec2[4] * vec1[2] - vec1[4] * vec2[2]) * r;

            // bitangent
            b[0] = (vec1[3] * vec2[0] - vec2[3] * vec1[0]) * r;
            b[1] = (vec1[3] * vec2[1] - vec2[3] * vec1[1]) * r;
            b[2] = (vec1[3] * vec2[2] - vec2[3] * vec1[2]) * r;

//             // invert T, B and N
//             r = 1.0f / ((t[0] * b[1] * plane[2] - t[2] * b[1] * plane[0]) +
//                         (b[0] * plane[1] * t[2] - b[2] * plane[1] * t[0]) +
//                         (plane[0] * t[1] * b[2] - plane[2] * t[1] * b[0]));
// 
//             polymer_crossproduct(b, plane, BxN);
//             polymer_crossproduct(plane, t, NxT);
//             polymer_crossproduct(t, b,     TxB);
// 
//             // GLSL matrix constructors are in column-major order
//             t[0] = BxN[0] * r;
//             t[1] = -NxT[0] * r;
//             t[2] = TxB[0] * r;
// 
//             b[0] = -BxN[1] * r;
//             b[1] = NxT[1] * r;
//             b[2] = -TxB[1] * r;
// 
//             n[0] = BxN[2] * r;
//             n[1] = -NxT[2] * r;
//             n[2] = TxB[2] * r;

            // normalize T, B and N
            norm = t[0] * t[0] + t[1] * t[1] + t[2] * t[2];
            norm = sqrt(norm);
            norm = 1.0 / norm;
            t[0] *= norm;
            t[1] *= norm;
            t[2] *= norm;

            norm = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
            norm = sqrt(norm);
            norm = 1.0 / norm;
            b[0] *= norm;
            b[1] *= norm;
            b[2] *= norm;

            norm = n[0] * n[0] + n[1] * n[1] + n[2] * n[2];
            norm = sqrt(norm);
            norm = 1.0 / norm;
            n[0] *= norm;
            n[1] *= norm;
            n[2] *= norm;

            break;
        }
        i+= 1;
    }
    while ((i + 2) < p->indicescount);
}

static inline void  polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLfloat* out)
{
    out[0] = in_a[1] * in_b[2] - in_a[2] * in_b[1];
    out[1] = in_a[2] * in_b[0] - in_a[0] * in_b[2];
    out[2] = in_a[0] * in_b[1] - in_a[1] * in_b[0];
}

static inline void  polymer_transformpoint(float* inpos, float* pos, float* matrix)
{
    pos[0] = inpos[0] * matrix[0] +
             inpos[1] * matrix[4] +
             inpos[2] * matrix[8] +
                      + matrix[12];
    pos[1] = inpos[0] * matrix[1] +
             inpos[1] * matrix[5] +
             inpos[2] * matrix[9] +
                      + matrix[13];
    pos[2] = inpos[0] * matrix[2] +
             inpos[1] * matrix[6] +
             inpos[2] * matrix[10] +
                      + matrix[14];
}

static inline void  polymer_pokesector(int16_t sectnum)
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
        frustum[i + 16] = matrix[(4 * i) + 3] - matrix[(4 * i) + 2];    // far
        i++;
    }
    i = 0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Frustum extracted.\n");
}

static int32_t      polymer_planeinfrustum(_prplane *plane, float* frustum)
{
    int32_t         i, j, k;
    float           sqdist;

    i = 0;
    while (i < 5)
    {
        j = k = 0;
        while (j < plane->vertcount)
        {
            sqdist = frustum[(i * 4) + 0] * plane->buffer[(j * 5) + 0] +
                     frustum[(i * 4) + 1] * plane->buffer[(j * 5) + 1] +
                     frustum[(i * 4) + 2] * plane->buffer[(j * 5) + 2] +
                     frustum[(i * 4) + 3];
            if (sqdist < 0)
                k++;
            j++;
        }
        if (k == plane->vertcount)
            return (0); // OUT !
        i++;
    }

    return (1);
}

static inline void  polymer_scansprites(int16_t sectnum, spritetype* localtsprite, int32_t* localspritesortcnt)
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
    float           pos[3];
    pthtyp*         pth;

    pos[0] = globalposy;
    pos[1] = -(float)(globalposz) / 16.0f;
    pos[2] = -globalposx;

    bglPushMatrix();
    bglLoadIdentity();
    bglLoadMatrixf(curskymodelviewmatrix);

    bglTranslatef(pos[0], pos[1], pos[2]);
    bglScalef(1000.0f, 1000.0f, 1000.0f);

    drawingskybox = 1;
    pth = gltexcache(tilenum,0,0);
    drawingskybox = 0;

    if (pth && (pth->flags & 4))
        polymer_drawskybox(tilenum);
    else
        polymer_drawartsky(tilenum);

    bglPopMatrix();
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
    float           *v0, *v1;
    md3surf_t       *s;
    char            lpal;
    float           spos[3], tspos[3], lpos[3], tlpos[3], vec[3];
    float           ang;
    float           scale;
    int32_t         surfi, i;
    GLfloat*        color;
    int32_t         materialbits;
    float           sradius, lradius;
    char            modellights[PR_MAXLIGHTS];
    char            modellightcount;

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
    bglLoadIdentity();
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

    // scripted model rotation
    if (spriteext[tspr->owner].pitch || spriteext[tspr->owner].roll)
    {
        float       pitchang, rollang, offsets[3];

        pitchang = (float)(spriteext[tspr->owner].pitch) / (2048.0f / 360.0f);
        rollang = (float)(spriteext[tspr->owner].roll) / (2048.0f / 360.0f);

        offsets[0] = -spriteext[tspr->owner].xoff / (scale * tspr->xrepeat);
        offsets[1] = -spriteext[tspr->owner].yoff / (scale * tspr->xrepeat);
        offsets[2] = (float)(spriteext[tspr->owner].zoff) / 16.0f / (scale * tspr->yrepeat);

        bglTranslatef(-offsets[0], -offsets[1], -offsets[2]);

        bglRotatef(pitchang, 0.0f, 1.0f, 0.0f);
        bglRotatef(rollang, -1.0f, 0.0f, 0.0f);

        bglTranslatef(offsets[0], offsets[1], offsets[2]);
    }

    bglGetFloatv(GL_MODELVIEW_MATRIX, spritemodelview);

    bglPopMatrix();
    bglPushMatrix();
    bglMultMatrixf(spritemodelview);

    // debug code for drawing the model bounding sphere
//     bglDisable(GL_TEXTURE_2D);
//     bglBegin(GL_LINES);
//     bglColor4f(1.0, 0.0, 0.0, 1.0);
//     bglVertex3f(m->head.frames[m->cframe].cen.x,
//                 m->head.frames[m->cframe].cen.y,
//                 m->head.frames[m->cframe].cen.z);
//     bglVertex3f(m->head.frames[m->cframe].cen.x + m->head.frames[m->cframe].r,
//                 m->head.frames[m->cframe].cen.y,
//                 m->head.frames[m->cframe].cen.z);
//     bglColor4f(0.0, 1.0, 0.0, 1.0);
//     bglVertex3f(m->head.frames[m->cframe].cen.x,
//                 m->head.frames[m->cframe].cen.y,
//                 m->head.frames[m->cframe].cen.z);
//     bglVertex3f(m->head.frames[m->cframe].cen.x,
//                 m->head.frames[m->cframe].cen.y + m->head.frames[m->cframe].r,
//                 m->head.frames[m->cframe].cen.z);
//     bglColor4f(0.0, 0.0, 1.0, 1.0);
//     bglVertex3f(m->head.frames[m->cframe].cen.x,
//                 m->head.frames[m->cframe].cen.y,
//                 m->head.frames[m->cframe].cen.z);
//     bglVertex3f(m->head.frames[m->cframe].cen.x,
//                 m->head.frames[m->cframe].cen.y,
//                 m->head.frames[m->cframe].cen.z + m->head.frames[m->cframe].r);
//     bglEnd();
//     bglEnable(GL_TEXTURE_2D);

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

    color[3] *=  (1.0f - spriteext[tspr->owner].alpha);

    if (tspr->cstat & 16384) color[3] = 0.0f;

    if (pr_gpusmoothing)
        mdspritematerial.frameprogress = m->interpol;

    mdspritematerial.mdspritespace = GL_TRUE;

    modellightcount = 0;

    // light culling
    if (lightcount)
    {
        sradius = (m->head.frames[m->cframe].r * (1 - m->interpol)) +
                  (m->head.frames[m->nframe].r * m->interpol);

        sradius *= max(scale * tspr->xrepeat, scale * tspr->yrepeat);
        sradius /= 1000.0f;

        spos[0] = (m->head.frames[m->cframe].cen.x * (1 - m->interpol)) +
                  (m->head.frames[m->nframe].cen.x * m->interpol);
        spos[1] = (m->head.frames[m->cframe].cen.y * (1 - m->interpol)) +
                  (m->head.frames[m->nframe].cen.y * m->interpol);
        spos[2] = (m->head.frames[m->cframe].cen.z * (1 - m->interpol)) +
                  (m->head.frames[m->nframe].cen.z * m->interpol);

        polymer_transformpoint(spos, tspos, spritemodelview);
        polymer_transformpoint(tspos, spos, rootmodelviewmatrix);

        i = 0;
        while (i < lightcount)
        {
            lradius = prlights[i].range / 1000.0f;

            lpos[0] = prlights[i].y;
            lpos[1] = -prlights[i].z / 16.0f;
            lpos[2] = -prlights[i].x;

            polymer_transformpoint(lpos, tlpos, rootmodelviewmatrix);

            vec[0] = tlpos[0] - spos[0];
            vec[0] *= vec[0];
            vec[1] = tlpos[1] - spos[1];
            vec[1] *= vec[1];
            vec[2] = tlpos[2] - spos[2];
            vec[2] *= vec[2];

            if ((vec[0] + vec[1] + vec[2]) <=
                ((sradius+lradius) * (sradius+lradius)))
            {
                modellights[modellightcount] = i;
                modellightcount++;
            }
            i++;
        }

    }

    for (surfi=0;surfi<m->head.numsurfs;surfi++)
    {
        s = &m->head.surfs[surfi];
        v0 = &s->geometry[m->cframe*s->numverts*6];
        v1 = &s->geometry[m->nframe*s->numverts*6];

        // debug code for drawing model normals
//         bglDisable(GL_TEXTURE_2D);
//         bglBegin(GL_LINES);
//         bglColor4f(1.0, 1.0, 1.0, 1.0);
// 
//         int i = 0;
//         while (i < s->numverts)
//         {
//             bglVertex3f(v0[(i * 6) + 0],
//                         v0[(i * 6) + 1],
//                         v0[(i * 6) + 2]);
//             bglVertex3f(v0[(i * 6) + 0] + v0[(i * 6) + 3] * 100,
//                         v0[(i * 6) + 1] + v0[(i * 6) + 4] * 100,
//                         v0[(i * 6) + 2] + v0[(i * 6) + 5] * 100);
//             i++;
//         }
//         bglEnd();
//         bglEnable(GL_TEXTURE_2D);


        mdspritematerial.diffusemap =
                mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,tspr->pal,surfi);
        if (!mdspritematerial.diffusemap)
            continue;

        if (!(tspr->cstat&1024))
        {
            mdspritematerial.detailmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,DETAILPAL,surfi);

            for (sk = m->skinmap; sk; sk = sk->next)
                if ((int32_t)sk->palette == DETAILPAL &&
                    sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                    sk->surfnum == surfi)
                    mdspritematerial.detailscale[0] = mdspritematerial.detailscale[1] = sk->param;
        }

        for (sk = m->skinmap; sk; sk = sk->next)
            if ((int32_t)sk->palette == tspr->pal &&
                 sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                 sk->surfnum == surfi)
        {
            if (sk->specpower != 1.0)
                mdspritematerial.specmaterial[0] = sk->specpower;
            mdspritematerial.specmaterial[1] = sk->specfactor;
        }

        if (!(tspr->cstat&1024))
        {
            mdspritematerial.glowmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,GLOWPAL,surfi);
        }

        bglEnableClientState(GL_NORMAL_ARRAY);

        if (pr_vbos > 1)
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->texcoords[surfi]);
            bglTexCoordPointer(2, GL_FLOAT, 0, 0);

            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->geometry[surfi]);
            bglVertexPointer(3, GL_FLOAT, sizeof(float) * 6, (GLfloat*)(m->cframe * s->numverts * sizeof(float) * 6));
            bglNormalPointer(GL_FLOAT, sizeof(float) * 6, (GLfloat*)(m->cframe * s->numverts * sizeof(float) * 6) + 3);

            if (pr_gpusmoothing)
            {
                mdspritematerial.nextframedata = (GLfloat*)(m->nframe * s->numverts * sizeof(float) * 6);
                mdspritematerial.nextframedatastride = sizeof(float) * 6;
            }

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m->indices[surfi]);

            curlight = 0;
            while ((curlight == 0) || ((curlight < modellightcount) && (curlight < pr_maxlightpasses)))
            {
                materialbits = polymer_bindmaterial(mdspritematerial, modellights, modellightcount);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, 0);
                polymer_unbindmaterial(materialbits);

                curlight++;
            }

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
        else
        {
            bglVertexPointer(3, GL_FLOAT, sizeof(float) * 6, v0);
            bglNormalPointer(GL_FLOAT, sizeof(float) * 6, v0 + 3);
            bglTexCoordPointer(2, GL_FLOAT, 0, s->uv);

            if (pr_gpusmoothing)
            {
                mdspritematerial.nextframedata = (GLfloat*)(v1);
                mdspritematerial.nextframedatastride = sizeof(float) * 6;
            }

            curlight = 0;
            while ((curlight == 0) || ((curlight < modellightcount) && (curlight < pr_maxlightpasses)))
            {
                materialbits = polymer_bindmaterial(mdspritematerial, modellights, modellightcount);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, s->tris);
                polymer_unbindmaterial(materialbits);

                curlight++;
            }
        }

        bglDisableClientState(GL_NORMAL_ARRAY);
    }

    bglPopMatrix();

    globalnoeffect=0;
}

static void         polymer_loadmodelvbos(md3model_t* m)
{
    int32_t         i;
    md3surf_t       *s;

    m->indices = Bcalloc(m->head.numsurfs, sizeof(GLuint));
    m->texcoords = Bcalloc(m->head.numsurfs, sizeof(GLuint));
    m->geometry = Bcalloc(m->head.numsurfs, sizeof(GLuint));

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
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, s->numframes * s->numverts * sizeof(float) * 6, s->geometry, modelvbousage);

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
    // PR_BIT_NORMAL_MAP
    material->normalmap = 0;
    material->normalbias[0] = material->normalbias[1] = 0.0f;
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
    // PR_BIT_SPECULAR_MAP
    material->specmap = 0;
    // PR_BIT_SPECULAR_MATERIAL
    material->specmaterial[0] = 15.0f;
    material->specmaterial[1] = 1.0f;
    // PR_BIT_MIRROR_MAP
    material->mirrormap = 0;
    // PR_BIT_GLOW_MAP
    material->glowmap = 0;
    // PR_BIT_SHADOW_MAP
    material->mdspritespace = GL_FALSE;
}

static void         polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade)
{
    pthtyp*         pth;
    pthtyp*         detailpth;
    pthtyp*         glowpth;

    polymer_getscratchmaterial(material);

    // PR_BIT_NORMAL_MAP
    if (hicfindsubst(tilenum, NORMALPAL, 0))
    {
        glowpth = NULL;
        glowpth = gltexcache(tilenum, NORMALPAL, 0);

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == NORMALPAL)) {
            material->normalmap = glowpth->glpic;
            material->normalbias[0] = glowpth->hicr->specpower;
            material->normalbias[1] = glowpth->hicr->specfactor;
        }
    }

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
    if (hicfindsubst(tilenum, DETAILPAL, 0))
    {
        detailpth = NULL;
        detailpth = gltexcache(tilenum, DETAILPAL, 0);

        if (detailpth && detailpth->hicr && (detailpth->hicr->palnum == DETAILPAL))
        {
            material->detailmap = detailpth->glpic;

            material->detailscale[0] = detailpth->hicr->xscale;
            material->detailscale[1] = detailpth->hicr->yscale;
        }
    }

    // PR_BIT_DIFFUSE_MODULATION
    material->diffusemodulation[0] =
            material->diffusemodulation[1] =
            material->diffusemodulation[2] =
            ((float)(numpalookups-min(max(shade*shadescale,0),numpalookups)))/((float)numpalookups);

    if (pth && (pth->flags & 2))
    {
        if (pth->palnum != pal)
        {
            material->diffusemodulation[0] *= (float)hictinting[pal].r / 255.0;
            material->diffusemodulation[1] *= (float)hictinting[pal].g / 255.0;
            material->diffusemodulation[2] *= (float)hictinting[pal].b / 255.0;
        }

        if (hictinting[MAXPALOOKUPS-1].r != 255 || hictinting[MAXPALOOKUPS-1].g != 255 || hictinting[MAXPALOOKUPS-1].b != 255)
        {
            material->diffusemodulation[0] *= (float)hictinting[MAXPALOOKUPS-1].r / 255.0;
            material->diffusemodulation[1] *= (float)hictinting[MAXPALOOKUPS-1].g / 255.0;
            material->diffusemodulation[2] *= (float)hictinting[MAXPALOOKUPS-1].b / 255.0;
        }
    }


    // PR_BIT_SPECULAR_MAP
    if (hicfindsubst(tilenum, SPECULARPAL, 0))
    {
        glowpth = NULL;
        glowpth = gltexcache(tilenum, SPECULARPAL, 0);

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == SPECULARPAL))
            material->specmap = glowpth->glpic;
    }

    // PR_BIT_SPECULAR_MATERIAL
    if (pth->hicr)
    {
        if (pth->hicr->specpower != 1.0f)
            material->specmaterial[0] = pth->hicr->specpower;
        material->specmaterial[1] = pth->hicr->specfactor;
    }

    // PR_BIT_GLOW_MAP
    if (r_fullbrights && pth && pth->flags & 16)
        material->glowmap = pth->ofb->glpic;

    if (hicfindsubst(tilenum, GLOWPAL, 0))
    {
        glowpth = NULL;
        glowpth = gltexcache(tilenum, GLOWPAL, 0);

        if (glowpth && glowpth->hicr && (glowpth->hicr->palnum == GLOWPAL))
            material->glowmap = glowpth->glpic;
    }
}

static int32_t      polymer_bindmaterial(_prmaterial material, char* lights, int lightcount)
{
    int32_t         programbits;
    int32_t         texunit;

    programbits = 0;

    // --------- bit validation

    // PR_BIT_ANIM_INTERPOLATION
    if (material.nextframedatastride)
        programbits |= prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit;

    // PR_BIT_LIGHTING_PASS
    if (curlight && lightcount)
        programbits |= prprogrambits[PR_BIT_LIGHTING_PASS].bit;

    // PR_BIT_NORMAL_MAP
    if (pr_normalmapping && material.normalmap)
        programbits |= prprogrambits[PR_BIT_NORMAL_MAP].bit;

    // PR_BIT_DIFFUSE_MAP
    if (material.diffusemap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_MAP].bit;

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (!curlight && r_detailmapping && material.detailmap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit;

    // PR_BIT_DIFFUSE_MODULATION
    programbits |= prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit;

    // PR_BIT_SPECULAR_MAP
    if (pr_specularmapping && material.specmap)
        programbits |= prprogrambits[PR_BIT_SPECULAR_MAP].bit;

    // PR_BIT_SPECULAR_MATERIAL
    if ((material.specmaterial[0] != 15.0) || (material.specmaterial[1] != 1.0) || pr_overridespecular)
        programbits |= prprogrambits[PR_BIT_SPECULAR_MATERIAL].bit;

    // PR_BIT_MIRROR_MAP
    if (!curlight && material.mirrormap)
        programbits |= prprogrambits[PR_BIT_MIRROR_MAP].bit;

    // PR_BIT_FOG
    programbits |= prprogrambits[PR_BIT_FOG].bit;

    // PR_BIT_GLOW_MAP
    if (!curlight && r_glowmapping && material.glowmap)
        programbits |= prprogrambits[PR_BIT_GLOW_MAP].bit;

    // PR_BIT_POINT_LIGHT
    if (lightcount) {
        programbits |= prprogrambits[PR_BIT_POINT_LIGHT].bit;
        // PR_BIT_SPOT_LIGHT
        if (prlights[lights[curlight]].radius) {
            programbits |= prprogrambits[PR_BIT_SPOT_LIGHT].bit;
            // PR_BIT_SHADOW_MAP
            if (prlights[lights[curlight]].rtindex != -1) {
                programbits |= prprogrambits[PR_BIT_SHADOW_MAP].bit;
                // PR_BIT_LIGHT_MAP
                if (prlights[lights[curlight]].lightmap)
                    programbits |= prprogrambits[PR_BIT_LIGHT_MAP].bit;
            }
        }
    }

    // material override
    programbits &= overridematerial;

    programbits |= prprogrambits[PR_BIT_HEADER].bit;
    programbits |= prprogrambits[PR_BIT_FOOTER].bit;

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
        if (prprograms[programbits].attrib_nextFrameNormal != -1)
            bglEnableVertexAttribArrayARB(prprograms[programbits].attrib_nextFrameNormal);
        bglVertexAttribPointerARB(prprograms[programbits].attrib_nextFrameData,
                                  3, GL_FLOAT, GL_FALSE,
                                  material.nextframedatastride,
                                  material.nextframedata);
        if (prprograms[programbits].attrib_nextFrameNormal != -1)
            bglVertexAttribPointerARB(prprograms[programbits].attrib_nextFrameNormal,
                                      3, GL_FLOAT, GL_FALSE,
                                      material.nextframedatastride,
                                      material.nextframedata + 3);

        bglUniform1fARB(prprograms[programbits].uniform_frameProgress, material.frameprogress);
    }

    // PR_BIT_LIGHTING_PASS
    if (programbits & prprogrambits[PR_BIT_LIGHTING_PASS].bit)
    {
        bglEnable(GL_BLEND);
        bglBlendFunc(GL_ONE, GL_ONE);
    }

    // PR_BIT_NORMAL_MAP
    if (programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
    {
        float pos[3], bias[2];

        pos[0] = globalposy;
        pos[1] = -(float)(globalposz) / 16.0f;
        pos[2] = -globalposx;

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.normalmap);

        bglUniform3fvARB(prprograms[programbits].uniform_eyePosition, 1, pos);
        bglUniform1iARB(prprograms[programbits].uniform_normalMap, texunit);
        if (pr_overrideparallax) {
            bias[0] = pr_parallaxscale;
            bias[1] = pr_parallaxbias;
            bglUniform2fvARB(prprograms[programbits].uniform_normalBias, 1, bias);
        } else
            bglUniform2fvARB(prprograms[programbits].uniform_normalBias, 1, material.normalbias);

        texunit++;
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
        float scale[2];

        // scale by the diffuse map scale if we're not doing normal mapping
        if (!(programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit))
        {
            scale[0] = material.diffusescale[0] * material.detailscale[0];
            scale[1] = material.diffusescale[1] * material.detailscale[1];
        } else {
            scale[0] = material.detailscale[0];
            scale[1] = material.detailscale[1];
        }

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.detailmap);

        bglUniform1iARB(prprograms[programbits].uniform_detailMap, texunit);
        bglUniform2fvARB(prprograms[programbits].uniform_detailScale, 1, scale);

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

    // PR_BIT_SPECULAR_MAP
    if (programbits & prprogrambits[PR_BIT_SPECULAR_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.specmap);

        bglUniform1iARB(prprograms[programbits].uniform_specMap, texunit);

        texunit++;
    }

    // PR_BIT_SPECULAR_MATERIAL
    if (programbits & prprogrambits[PR_BIT_SPECULAR_MATERIAL].bit)
    {
        float specmaterial[2];

        if (pr_overridespecular) {
            specmaterial[0] = pr_specularpower;
            specmaterial[1] = pr_specularfactor;
            bglUniform2fvARB(prprograms[programbits].uniform_specMaterial, 1, specmaterial);
        } else
            bglUniform2fvARB(prprograms[programbits].uniform_specMaterial, 1, material.specmaterial);
    }

    // PR_BIT_MIRROR_MAP
    if (programbits & prprogrambits[PR_BIT_MIRROR_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_RECTANGLE, material.mirrormap);

        bglUniform1iARB(prprograms[programbits].uniform_mirrorMap, texunit);

        texunit++;
    }

    // PR_BIT_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_GLOW_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material.glowmap);

        bglUniform1iARB(prprograms[programbits].uniform_glowMap, texunit);

        texunit++;
    }

    // PR_BIT_POINT_LIGHT
    if (programbits & prprogrambits[PR_BIT_POINT_LIGHT].bit)
    {
        float inpos[4], pos[4];
        float range[2];
        float color[4];

        inpos[0] = prlights[lights[curlight]].y;
        inpos[1] = -prlights[lights[curlight]].z / 16.0f;
        inpos[2] = -prlights[lights[curlight]].x;

        polymer_transformpoint(inpos, pos, curmodelviewmatrix);

        // PR_BIT_SPOT_LIGHT
        if (programbits & prprogrambits[PR_BIT_SPOT_LIGHT].bit)
        {
            float sinang, cosang, sinhorizang, coshorizangs;
            float indir[3], dir[3];

            cosang = (float)(sintable[(-prlights[lights[curlight]].angle+1024)&2047]) / 16383.0f;
            sinang = (float)(sintable[(-prlights[lights[curlight]].angle+512)&2047]) / 16383.0f;
            coshorizangs = (float)(sintable[(getangle(128, prlights[lights[curlight]].horiz-100)+1024)&2047]) / 16383.0f;
            sinhorizang = (float)(sintable[(getangle(128, prlights[lights[curlight]].horiz-100)+512)&2047]) / 16383.0f;

            indir[0] = inpos[0] + sinhorizang * cosang;
            indir[1] = inpos[1] - coshorizangs;
            indir[2] = inpos[2] - sinhorizang * sinang;

            polymer_transformpoint(indir, dir, curmodelviewmatrix);

            dir[0] -= pos[0];
            dir[1] -= pos[1];
            dir[2] -= pos[2];

            indir[0] = (float)(sintable[(prlights[lights[curlight]].radius+512)&2047]) / 16383.0f;
            indir[1] = (float)(sintable[(prlights[lights[curlight]].faderadius+512)&2047]) / 16383.0f;
            indir[1] = 1.0 / (indir[1] - indir[0]);

            bglUniform3fvARB(prprograms[programbits].uniform_spotDir, 1, dir);
            bglUniform2fvARB(prprograms[programbits].uniform_spotRadius, 1, indir);

            // PR_BIT_SHADOW_MAP
            if (programbits & prprogrambits[PR_BIT_SHADOW_MAP].bit)
            {
                GLfloat matrix[16];

                bglMatrixMode(GL_TEXTURE);
                bglLoadMatrixf(shadowBias);
                bglMultMatrixf(prlights[lights[curlight]].proj);
                bglMultMatrixf(prlights[lights[curlight]].transform);
                if (material.mdspritespace == GL_TRUE)
                    bglMultMatrixf(spritemodelview);
                bglGetFloatv(GL_TEXTURE_MATRIX, matrix);
                bglLoadIdentity();
                bglMatrixMode(GL_MODELVIEW);

                bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
                bglBindTexture(prrts[prlights[lights[curlight]].rtindex].target, prrts[prlights[lights[curlight]].rtindex].z);

                bglUniform1iARB(prprograms[programbits].uniform_shadowMap, texunit);
                bglUniformMatrix4fvARB(prprograms[programbits].uniform_shadowProjMatrix, 1, GL_FALSE, matrix);

                texunit++;

                // PR_BIT_LIGHT_MAP
                if (programbits & prprogrambits[PR_BIT_LIGHT_MAP].bit)
                {
                    bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
                    bglBindTexture(GL_TEXTURE_2D, prlights[lights[curlight]].lightmap);

                    bglUniform1iARB(prprograms[programbits].uniform_lightMap, texunit);

                    texunit++;
                }
            }
        }

        range[0] = prlights[lights[curlight]].range  / 1000.0f;
        range[1] = 1 / range[0];

        color[0] = prlights[lights[curlight]].color[0]   / 255.0f;
        color[1] = prlights[lights[curlight]].color[1]   / 255.0f;
        color[2] = prlights[lights[curlight]].color[2]   / 255.0f;

        bglLightfv(GL_LIGHT0, GL_AMBIENT, pos);
        bglLightfv(GL_LIGHT0, GL_DIFFUSE, color);
        bglLightfv(GL_LIGHT0, GL_SPECULAR, inpos);
        bglLightfv(GL_LIGHT0, GL_CONSTANT_ATTENUATION, &range[0]);
        bglLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, &range[1]);
    }

    bglActiveTextureARB(GL_TEXTURE0_ARB);

    return (programbits);
}

static void         polymer_unbindmaterial(int32_t programbits)
{
    // repair any dirty GL state here

    // PR_BIT_ANIM_INTERPOLATION
    if (programbits & prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit)
    {
        if (prprograms[programbits].attrib_nextFrameNormal != -1)
            bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_nextFrameNormal);
        bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_nextFrameData);
    }

    // PR_BIT_LIGHTING_PASS
    if (programbits & prprogrambits[PR_BIT_LIGHTING_PASS].bit)
    {
        bglDisable(GL_BLEND);
        bglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    bglUseProgramObjectARB(0);
}

static void         polymer_compileprogram(int32_t programbits)
{
    int32_t         i, enabledbits;
    GLhandleARB     vert, frag, program;
    GLcharARB*      source[PR_BIT_COUNT * 2];
    GLcharARB       infobuffer[PR_INFO_LOG_BUFFER_SIZE];
    GLint           linkstatus;

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

    bglGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linkstatus);

    bglGetInfoLogARB(program, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);

    prprograms[programbits].handle = program;

    if (pr_verbosity >= 1) OSD_Printf("PR : Compiling GPU program with bits %i...\n", programbits);
    if (!linkstatus) {
        OSD_Printf("PR : Failed to compile GPU program with bits %i!\n", programbits);
        if (pr_verbosity >= 1) OSD_Printf("PR : Compilation log:\n%s\n", infobuffer);
        bglGetShaderSourceARB(vert, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);
        if (pr_verbosity >= 1) OSD_Printf("PR : Vertex source dump:\n%s\n", infobuffer);
        bglGetShaderSourceARB(frag, PR_INFO_LOG_BUFFER_SIZE, NULL, infobuffer);
        if (pr_verbosity >= 1) OSD_Printf("PR : Fragment source dump:\n%s\n", infobuffer);
    }

    // --------- ATTRIBUTE/UNIFORM LOCATIONS

    // PR_BIT_ANIM_INTERPOLATION
    if (programbits & prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit)
    {
        prprograms[programbits].attrib_nextFrameData = bglGetAttribLocationARB(program, "nextFrameData");
        prprograms[programbits].attrib_nextFrameNormal = bglGetAttribLocationARB(program, "nextFrameNormal");
        prprograms[programbits].uniform_frameProgress = bglGetUniformLocationARB(program, "frameProgress");
    }

    // PR_BIT_NORMAL_MAP
    if (programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
    {
        prprograms[programbits].attrib_T = bglGetAttribLocationARB(program, "T");
        prprograms[programbits].attrib_B = bglGetAttribLocationARB(program, "B");
        prprograms[programbits].attrib_N = bglGetAttribLocationARB(program, "N");
        prprograms[programbits].uniform_eyePosition = bglGetUniformLocationARB(program, "eyePosition");
        prprograms[programbits].uniform_normalMap = bglGetUniformLocationARB(program, "normalMap");
        prprograms[programbits].uniform_normalBias = bglGetUniformLocationARB(program, "normalBias");
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

    // PR_BIT_SPECULAR_MAP
    if (programbits & prprogrambits[PR_BIT_SPECULAR_MAP].bit)
    {
        prprograms[programbits].uniform_specMap = bglGetUniformLocationARB(program, "specMap");
    }

    // PR_BIT_SPECULAR_MATERIAL
    if (programbits & prprogrambits[PR_BIT_SPECULAR_MATERIAL].bit)
    {
        prprograms[programbits].uniform_specMaterial = bglGetUniformLocationARB(program, "specMaterial");
    }

    // PR_BIT_MIRROR_MAP
    if (programbits & prprogrambits[PR_BIT_MIRROR_MAP].bit)
    {
        prprograms[programbits].uniform_mirrorMap = bglGetUniformLocationARB(program, "mirrorMap");
    }

    // PR_BIT_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_GLOW_MAP].bit)
    {
        prprograms[programbits].uniform_glowMap = bglGetUniformLocationARB(program, "glowMap");
    }

    // PR_BIT_SHADOW_MAP
    if (programbits & prprogrambits[PR_BIT_SHADOW_MAP].bit)
    {
        prprograms[programbits].uniform_shadowMap = bglGetUniformLocationARB(program, "shadowMap");
        prprograms[programbits].uniform_shadowProjMatrix = bglGetUniformLocationARB(program, "shadowProjMatrix");
    }

    // PR_BIT_LIGHT_MAP
    if (programbits & prprogrambits[PR_BIT_LIGHT_MAP].bit)
    {
        prprograms[programbits].uniform_lightMap = bglGetUniformLocationARB(program, "lightMap");
    }

    // PR_BIT_SPOT_LIGHT
    if (programbits & prprogrambits[PR_BIT_SPOT_LIGHT].bit)
    {
        prprograms[programbits].uniform_spotDir = bglGetUniformLocationARB(program, "spotDir");
        prprograms[programbits].uniform_spotRadius = bglGetUniformLocationARB(program, "spotRadius");
    }
}

// LIGHTS
static void         polymer_resetlights(void)
{
    int32_t         i;
    _prsector       *s;
    _prwall         *w;

    i = 0;
    while (i < numsectors)
    {
        s = prsectors[i];

        s->floor.lightcount = 0;
        s->ceil.lightcount = 0;

        i++;
    }

    i = 0;
    while (i < numwalls)
    {
        w = prwalls[i];

        w->wall.lightcount = 0;
        w->over.lightcount = 0;
        w->mask.lightcount = 0;

        i++;
    }

    lightcount = 0;
}

static void         polymer_addlight(_prlight light)
{
    if ((light.sector == -1) || (light.sector >= numsectors))
        return;

    if (lightcount < PR_MAXLIGHTS)
    {
        prlights[lightcount] = light;

        if (light.radius) {
            float           radius, ang, horizang, lightpos[3];
            pthtyp*         pth;

            // hack to avoid lights beams perpendicular to walls
            if ((light.horiz <= 100) && (light.horiz > 90))
                light.horiz = 90;
            if ((light.horiz > 100) && (light.horiz < 110))
                light.horiz = 110;

            lightpos[0] = light.y;
            lightpos[1] = -light.z / 16.0f;
            lightpos[2] = -light.x;

            // calculate the spot light transformations and matrices
            radius = (float)(light.radius) / (2048.0f / 360.0f);
            ang = (float)(light.angle) / (2048.0f / 360.0f);
            horizang = (float)(-getangle(128, light.horiz-100)) / (2048.0f / 360.0f);

            bglMatrixMode(GL_PROJECTION);
            bglPushMatrix();
            bglLoadIdentity();
            bgluPerspective(radius * 2, 1, 0.1f, light.range / 1000.0f);
            bglGetFloatv(GL_PROJECTION_MATRIX, prlights[lightcount].proj);
            bglPopMatrix();

            bglMatrixMode(GL_MODELVIEW);
            bglPushMatrix();
            bglLoadIdentity();
            bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
            bglRotatef(ang, 0.0f, 1.0f, 0.0f);
            bglScalef(1.0f / 1000.0f, 1.0f / 1000.0f, 1.0f / 1000.0f);
            bglTranslatef(-lightpos[0], -lightpos[1], -lightpos[2]);
            bglGetFloatv(GL_MODELVIEW_MATRIX, prlights[lightcount].transform);
            bglPopMatrix();

            polymer_extractfrustum(prlights[lightcount].transform, prlights[lightcount].proj, prlights[lightcount].frustum);

            prlights[lightcount].rtindex = -1;

            // get the texture handle for the lightmap
            prlights[lightcount].lightmap = 0;
            if (prlights[lightcount].tilenum)
            {
                if (!waloff[prlights[lightcount].tilenum])
                    loadtile(prlights[lightcount].tilenum);

                pth = NULL;
                pth = gltexcache(prlights[lightcount].tilenum, 0, 0);

                if (pth)
                    prlights[lightcount].lightmap = pth->glpic;
            }
        }

        prlights[lightcount].isinview = 0;

        polymer_culllight(lightcount);

        lightcount++;
    }
}

static int32_t      polymer_planeinlight(_prplane* plane, _prlight* light)
{
    float           lightpos[3];
    int             i, j, k, l;

    if (!plane->vertcount)
        return 0;

    if (light->radius)
        return polymer_planeinfrustum(plane, light->frustum);

    lightpos[0] = light->y;
    lightpos[1] = -light->z / 16.0f;
    lightpos[2] = -light->x;

    i = 0;

    while (i < 3)
    {
        j = k = l = 0;

        while (j < plane->vertcount)
        {
            if (plane->buffer[(j * 5) + i] > (lightpos[i] + light->range)) k++;
            if (plane->buffer[(j * 5) + i] < (lightpos[i] - light->range)) l++;
            j++;
        }

        if ((k == plane->vertcount) || (l == plane->vertcount))
            return 0;

        i++;
    }

    return 1;
}

static inline void  polymer_culllight(char lightindex)
{
    _prlight*       light;
    int32_t         front;
    int32_t         back;
    int32_t         i;
    _prsector       *s;
    _prwall         *w;
    sectortype      *sec;

    light = &prlights[lightindex];
    front = 0;
    back = 1;
    Bmemset(drawingstate, 0, sizeof(int16_t) * numsectors);
    drawingstate[light->sector] = 1;

    sectorqueue[0] = light->sector;

    while (front != back)
    {
        s = prsectors[sectorqueue[front]];
        sec = &sector[sectorqueue[front]];

        polymer_pokesector(sectorqueue[front]);

        if (polymer_planeinlight(&s->floor, light)) {
            s->floor.lights[s->floor.lightcount] = lightindex;
            s->floor.lightcount++;
            if (s->floor.drawn)
                light->isinview = 1;
        }
        if (polymer_planeinlight(&s->ceil, light)) {
            s->ceil.lights[s->ceil.lightcount] = lightindex;
            s->ceil.lightcount++;
            if (s->ceil.drawn)
                light->isinview = 1;
        }

        i = 0;
        while (i < sec->wallnum)
        {
            w = prwalls[sec->wallptr + i];

            if (polymer_planeinlight(&w->wall, light)) {
                w->wall.lights[w->wall.lightcount] = lightindex;
                w->wall.lightcount++;
                if (w->wall.drawn)
                    light->isinview = 1;
            }
            if (polymer_planeinlight(&w->over, light)) {
                w->over.lights[w->over.lightcount] = lightindex;
                w->over.lightcount++;
                if (w->over.drawn)
                    light->isinview = 1;
            }
            if (polymer_planeinlight(&w->mask, light)) {
                if ((w->mask.vertcount == 4) &&
                    (w->mask.buffer[(0 * 5) + 1] >= w->mask.buffer[(3 * 5) + 1]) &&
                    (w->mask.buffer[(1 * 5) + 1] >= w->mask.buffer[(2 * 5) + 1]))
                {
                    i++;
                    continue;
                }

                w->mask.lights[w->mask.lightcount] = lightindex;
                w->mask.lightcount++;
                if (w->mask.drawn)
                    light->isinview = 1;

                if ((wall[sec->wallptr + i].nextsector != -1) &&
                    (!drawingstate[wall[sec->wallptr + i].nextsector])) {
                    drawingstate[wall[sec->wallptr + i].nextsector] = 1;
                    sectorqueue[back] = wall[sec->wallptr + i].nextsector;
                    back++;
                }
            }

            i++;
        }
        front++;
    }
}

static void         polymer_prepareshadows(void)
{
    int16_t         oviewangle, oglobalang;
    int32_t         ocosglobalang, osinglobalang;
    int32_t         ocosviewingrangeglobalang, osinviewingrangeglobalang;
    int32_t         i, j;
    int32_t         gx, gy, gz;
    int32_t         oldoverridematerial;

    // for wallvisible()
    gx = globalposx;
    gy = globalposy;
    gz = globalposz;
    // build globals used by drawmasks
    oviewangle = viewangle;
    oglobalang = globalang;
    ocosglobalang = cosglobalang;
    osinglobalang = singlobalang;
    ocosviewingrangeglobalang = cosviewingrangeglobalang;
    osinviewingrangeglobalang = sinviewingrangeglobalang;

    i = j = 0;

    while ((i < lightcount) && (j < pr_shadowcount))
    {
        if (prlights[i].radius && prlights[i].isinview)
        {
            prlights[i].rtindex = j + 1;

            bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, prrts[prlights[i].rtindex].fbo);
            bglPushAttrib(GL_VIEWPORT_BIT);
            bglViewport(0, 0, prrts[prlights[i].rtindex].xdim, prrts[prlights[i].rtindex].ydim);

            bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            bglMatrixMode(GL_PROJECTION);
            bglPushMatrix();
            bglLoadMatrixf(prlights[i].proj);
            bglMatrixMode(GL_MODELVIEW);
            bglLoadMatrixf(prlights[i].transform);

            bglEnable(GL_POLYGON_OFFSET_FILL);
            bglPolygonOffset(5, SHADOW_DEPTH_OFFSET);

            globalposx = prlights[i].x;
            globalposy = prlights[i].y;
            globalposz = prlights[i].z;

            // build globals used by rotatesprite
            viewangle = prlights[i].angle;
            globalang = (prlights[i].angle&2047);
            cosglobalang = sintable[(globalang+512)&2047];
            singlobalang = sintable[globalang&2047];
            cosviewingrangeglobalang = mulscale16(cosglobalang,viewingrange);
            sinviewingrangeglobalang = mulscale16(singlobalang,viewingrange);

            oldoverridematerial = overridematerial;
            // smooth model shadows
            overridematerial = prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit;
            // used by alpha-testing for sprite silhouette
            overridematerial |= prprogrambits[PR_BIT_DIFFUSE_MAP].bit;

            // to force sprite drawing
            mirrors[depth++].plane = NULL;
            polymer_displayrooms(prlights[i].sector);
            depth--;

            overridematerial = oldoverridematerial;

            bglDisable(GL_POLYGON_OFFSET_FILL);

            bglMatrixMode(GL_PROJECTION);
            bglPopMatrix();

            bglPopAttrib();
            bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

            j++;
        }
        i++;
    }

    globalposx = gx;
    globalposy = gy;
    globalposz = gz;

    viewangle = oviewangle;
    globalang = oglobalang;
    cosglobalang = ocosglobalang;
    singlobalang = osinglobalang;
    cosviewingrangeglobalang = ocosviewingrangeglobalang;
    sinviewingrangeglobalang = osinviewingrangeglobalang;
}

static void         polymer_applylights(void)
{
    int32_t         i, curpriority;
    _prlight        light;
    float           fade;

    curpriority = 0;
    while (curpriority < PR_MAXLIGHTPRIORITY)
    {
        i = 0;
        while (i < staticlightcount)
        {
            if ((staticlights[i].priority != curpriority) ||
                (staticlights[i].priority > pr_maxlightpriority))
            {
                i++;
                continue;
            }

            if (staticlights[i].minshade == staticlights[i].maxshade)
                polymer_addlight(staticlights[i]);
            else {
                light = staticlights[i];

                fade = sector[light.sector].floorshade;
                fade -= light.minshade;
                fade /= light.maxshade - light.minshade;

                if (fade < 0.0f)
                    fade = 0.0f;
                if (fade > 1.0f)
                    fade = 1.0f;

                light.color[0] *= fade;
                light.color[1] *= fade;
                light.color[2] *= fade;

                polymer_addlight(light);
            }
            i++;
        }

        i = 0;
        while (i < gamelightcount)
        {
            if ((gamelights[i].priority != curpriority) ||
                (gamelights[i].priority > pr_maxlightpriority))
            {
                i++;
                continue;
            }

            polymer_addlight(gamelights[i]);
            i++;
        }

        i = 0;
        while (i < framelightcount)
        {
            if ((framelights[i].priority != curpriority) ||
                (framelights[i].priority > pr_maxlightpriority))
            {
                i++;
                continue;
            }

            polymer_addlight(framelights[i]);
            i++;
        }
        curpriority++;
    }
}

// RENDER TARGETS
static void         polymer_initrendertargets(int32_t count)
{
    int32_t         i;

    prrts = Bcalloc(count, sizeof(_prrt));

    i = 0;
    while (i < count)
    {
        if (!i) {
            prrts[i].target = GL_TEXTURE_RECTANGLE;
            prrts[i].xdim = xdim;
            prrts[i].ydim = ydim;

            bglGenTextures(1, &prrts[i].color);
            bglBindTexture(prrts[i].target, prrts[i].color);

            bglTexImage2D(prrts[i].target, 0, GL_RGBA, prrts[i].xdim, prrts[i].ydim, 0, GL_RGBA, GL_SHORT, NULL);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, GL_CLAMP);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, GL_CLAMP);
        } else {
            prrts[i].target = GL_TEXTURE_2D;
            prrts[i].xdim = 128 << pr_shadowdetail;
            prrts[i].ydim = 128 << pr_shadowdetail;
            prrts[i].color = 0;

            bglGenTextures(1, &prrts[i].color);
            bglBindTexture(prrts[i].target, prrts[i].color);

            bglTexImage2D(prrts[i].target, 0, GL_RGBA, prrts[i].xdim, prrts[i].ydim, 0, GL_RGBA, GL_SHORT, NULL);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, GL_CLAMP);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }

        bglGenTextures(1, &prrts[i].z);
        bglBindTexture(prrts[i].target, prrts[i].z);

        bglTexImage2D(prrts[i].target, 0, GL_DEPTH_COMPONENT, prrts[i].xdim, prrts[i].ydim, 0, GL_DEPTH_COMPONENT, GL_SHORT, NULL);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, GL_CLAMP);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, GL_CLAMP);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);
        bglTexParameteri(prrts[i].target, GL_DEPTH_TEXTURE_MODE_ARB, GL_ALPHA);

        bglGenFramebuffersEXT(1, &prrts[i].fbo);
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, prrts[i].fbo);

        if (prrts[i].color)
            bglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                       prrts[i].target, prrts[i].color, 0);
        else {
            bglDrawBuffer(GL_NONE);
            bglReadBuffer(GL_NONE);
        }
        bglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, prrts[i].target, prrts[i].z, 0);

        if (bglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            OSD_Printf("PR : FBO #%d initialization failed.\n", i);
        }

        bglBindTexture(prrts[i].target, 0);
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        i++;
    }
}

#endif
