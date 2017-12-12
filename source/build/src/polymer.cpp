// blah

#if defined USE_OPENGL && defined POLYMER

#include "compat.h"

#define POLYMER_C
#include "polymer.h"
#include "engine_priv.h"
#include "xxhash.h"
#include "texcache.h"

// CVARS
int32_t         pr_lighting = 1;
int32_t         pr_normalmapping = 1;
int32_t         pr_specularmapping = 1;
int32_t         pr_shadows = 1;
int32_t         pr_shadowcount = 5;
int32_t         pr_shadowdetail = 4;
int32_t         pr_shadowfiltering = 1;
int32_t         pr_maxlightpasses = 10;
int32_t         pr_maxlightpriority = PR_MAXLIGHTPRIORITY;
int32_t         pr_fov = 426;           // appears to be the classic setting.
double          pr_customaspect = 0.0f;
int32_t         pr_billboardingmode = 1;
int32_t         pr_verbosity = 1;       // 0: silent, 1: errors and one-times, 2: multiple-times, 3: flood
int32_t         pr_wireframe = 0;
int32_t         pr_vbos = 2;
int32_t         pr_buckets = 0;
int32_t         pr_gpusmoothing = 1;
int32_t         pr_overrideparallax = 0;
float           pr_parallaxscale = 0.1f;
float           pr_parallaxbias = 0.0f;
int32_t         pr_overridespecular = 0;
float           pr_specularpower = 15.0f;
float           pr_specularfactor = 1.0f;
int32_t         pr_highpalookups = 1;
int32_t         pr_artmapping = 1;
int32_t         pr_overridehud = 0;
float           pr_hudxadd = 0.0f;
float           pr_hudyadd = 0.0f;
float           pr_hudzadd = 0.0f;
int32_t         pr_hudangadd = 0;
int32_t         pr_hudfov = 426;
float           pr_overridemodelscale = 0.0f;
int32_t         pr_ati_fboworkaround = 0;
int32_t         pr_ati_nodepthoffset = 0;
#ifdef __APPLE__
int32_t         pr_ati_textureformat_one = 0;
#endif
int32_t         pr_nullrender = 0; // 1: no draw, 2: no draw or updates

int32_t         r_pr_maxlightpasses = 5; // value of the cvar (not live value), used to detect changes

GLenum          mapvbousage = GL_STREAM_DRAW_ARB;
GLenum          modelvbousage = GL_STATIC_DRAW_ARB;

// BUILD DATA
_prsector       *prsectors[MAXSECTORS];
_prwall         *prwalls[MAXWALLS];
_prsprite       *prsprites[MAXSPRITES];
_prmaterial     mdspritematerial;
_prhighpalookup prhighpalookups[MAXBASEPALS][MAXPALOOKUPS];

// One U8 texture per tile
GLuint          prartmaps[MAXTILES];
// 256 U8U8U8 values per basepal
GLuint          prbasepalmaps[MAXBASEPALS];
// numshades full indirections (32*256) per lookup
GLuint          prlookups[MAXPALOOKUPS];

GLuint          prmapvbo;
const GLsizeiptrARB proneplanesize = sizeof(_prvert) * 4;
const GLintptrARB prwalldatasize = sizeof(_prvert)* 4 * 3; // wall, over and mask planes for every wall
GLintptrARB prwalldataoffset;

GLuint          prindexringvbo;
GLuint          *prindexring;
const GLsizeiptrARB prindexringsize = 65535;
GLintptrARB prindexringoffset;

const GLbitfield prindexringmapflags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

_prbucket       *prbuckethead;
int32_t         prcanbucket;

static const _prvert  vertsprite[4] =
{
    {
        -0.5f, 0.0f, 0.0f,
        0.0f, 1.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        0.5f, 0.0f, 0.0f,
        1.0f, 1.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        0.5f, 1.0f, 0.0f,
        1.0f, 0.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        -0.5f, 1.0f, 0.0f,
        0.0f, 0.0f,
        0xff, 0xff, 0xff, 0xff,
    },
};

static const _prvert  horizsprite[4] =
{
    {
        -0.5f, 0.0f, 0.5f,
        0.0f, 0.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        0.5f, 0.0f, 0.5f,
        1.0f, 0.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        0.5f, 0.0f, -0.5f,
        1.0f, 1.0f,
        0xff, 0xff, 0xff, 0xff,
    },
    {
        -0.5f, 0.0f, -0.5f,
        0.0f, 1.0f,
        0xff, 0xff, 0xff, 0xff,
    },
};

static const GLfloat  skyboxdata[4 * 5 * 6] =
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
static _prplanelist *plpool;
#pragma pack(push,1)
_prlight        prlights[PR_MAXLIGHTS];
int32_t         lightcount;
int32_t         curlight;
#pragma pack(pop)

static const GLfloat  shadowBias[] =
{
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
};

// MATERIALS
static const _prprogrambit   prprogrambits[PR_BIT_COUNT] = {
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
        "  TBN = mat3(T, B, N);\n"
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
        1 << PR_BIT_ART_MAP,
        // vert_def
        "varying vec3 horizDistance;\n"
        "\n",
        // vert_prog
        "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "  horizDistance = vec3(gl_ModelViewMatrix * curVertex);\n"
        "\n",
        // frag_def
        "uniform sampler2D artMap;\n"
        "uniform sampler2D basePalMap;\n"
        "uniform sampler2DRect lookupMap;\n"
        "uniform float shadeOffset;\n"
        "uniform float visibility;\n"
        "varying vec3 horizDistance;\n"
        "\n",
        // frag_prog

        // NOTE: the denominator was 1.024, but we increase it towards a bit
        // farther far clipoff distance to account for the fact that the
        // distance to the fragment is the common Euclidean one, as opposed to
        // the "ortho" distance of Build.
        "  float shadeLookup = length(horizDistance) / 1.07 * visibility;\n"
        "  shadeLookup = shadeLookup + shadeOffset;\n"
        "\n"
        "  float colorIndex = texture2D(artMap, commonTexCoord.st).r * 256.0;\n"
        "  float colorIndexNear = texture2DRect(lookupMap, vec2(colorIndex, floor(shadeLookup))).r;\n"
        "  float colorIndexFar = texture2DRect(lookupMap, vec2(colorIndex, floor(shadeLookup + 1.0))).r;\n"
        "  float colorIndexFullbright = texture2DRect(lookupMap, vec2(colorIndex, 0.0)).r;\n"
        "\n"
        "  vec3 texelNear = texture2D(basePalMap, vec2(colorIndexNear, 0.5)).rgb;\n"
        "  vec3 texelFar = texture2D(basePalMap, vec2(colorIndexFar, 0.5)).rgb;\n"
        "  diffuseTexel.rgb = texture2D(basePalMap, vec2(colorIndexFullbright, 0.5)).rgb;\n"
        "\n"
        "  if (isLightingPass == 0) {\n"
        "    result.rgb = mix(texelNear, texelFar, fract(shadeLookup));\n"
        "    result.a = 1.0;\n"
        "    if (colorIndex == 256.0)\n"
        "      result.a = 0.0;\n"
        "  }\n"
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
        "    diffuseTexel *= texture2D(detailMap, gl_TexCoord[1].st);\n"
        "  else\n"
        "    diffuseTexel *= texture2D(detailMap, commonTexCoord.st * fragDetailScale);\n"
        "  diffuseTexel.rgb *= 2.0;\n"
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
        1 << PR_BIT_DIFFUSE_MAP2,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "",
        // frag_prog
        "  if (isLightingPass == 0)\n"
        "    result *= diffuseTexel;\n"
        "\n",
    },
    {
        1 << PR_BIT_HIGHPALOOKUP_MAP,
        // vert_def
        "",
        // vert_prog
        "",
        // frag_def
        "uniform sampler3D highPalookupMap;\n"
        "\n",
        // frag_prog
        "  float highPalScale = 0.9921875; // for 6 bits\n"
        "  float highPalBias = 0.00390625;\n"
        "\n"
        "  if (isLightingPass == 0)\n"
        "    result.rgb = texture3D(highPalookupMap, result.rgb * highPalScale + highPalBias).rgb;\n"
        "  diffuseTexel.rgb = texture3D(highPalookupMap, diffuseTexel.rgb * highPalScale + highPalBias).rgb;\n"
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
#ifdef PR_LINEAR_FOG
        "uniform bool linearFog;\n"
#endif
        "",
        // frag_prog
        "  float fragDepth;\n"
        "  float fogFactor;\n"
        "\n"
        "  fragDepth = gl_FragCoord.z / gl_FragCoord.w / 35.0;\n"
#ifdef PR_LINEAR_FOG
        "  if (!linearFog) {\n"
#endif
        "    fragDepth *= fragDepth;\n"
        "    fogFactor = exp2(-gl_Fog.density * gl_Fog.density * fragDepth * 1.442695);\n"
#ifdef PR_LINEAR_FOG
        /* 0.65127==150/230, another constant found out by experiment. :/
         * (150 is Polymost's old FOGDISTCONST.) */
        "  } else {\n"
        "    fogFactor = gl_Fog.scale * (gl_Fog.end - fragDepth*0.65217);\n"
        "    fogFactor = clamp(fogFactor, 0.0, 1.0);"
        "  }\n"
#endif
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
        1 << PR_BIT_PROJECTION_MAP,
        // vert_def
        "uniform mat4 shadowProjMatrix;\n"
        "\n",
        // vert_prog
        "  gl_TexCoord[2] = shadowProjMatrix * curVertex;\n"
        "\n",
        // frag_def
        "",
        // frag_prog
        "",
    },
    {
        1 << PR_BIT_SHADOW_MAP,
        // vert_def
        "",
        // vert_prog
        "",
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
        "  lightTexel = texture2D(lightMap, vec2(gl_TexCoord[2].s, -gl_TexCoord[2].t) / gl_TexCoord[2].q).rgb;\n"
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
        "  pointLightDistance = dot(lightVector,lightVector);\n"
        "  lightAttenuation = clamp(1.0 - pointLightDistance * gl_LightSource[0].linearAttenuation, 0.0, 1.0);\n"
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
int32_t         globaloldoverridematerial;

int32_t         rotatespritematerialbits;

// RENDER TARGETS
_prrt           *prrts;

// CONTROL
GLfloat         spritemodelview[16];
GLfloat         mdspritespace[4][4];
GLfloat         rootmodelviewmatrix[16];
GLfloat         *curmodelviewmatrix;
GLfloat         rootskymodelviewmatrix[16];
GLfloat         *curskymodelviewmatrix;

static int16_t  sectorqueue[MAXSECTORS];
static int16_t  querydelay[MAXSECTORS];
static GLuint   queryid[MAXWALLS];
static int16_t  drawingstate[MAXSECTORS];

int16_t         *cursectormasks;
int16_t         *cursectormaskcount;

float           horizang;
int16_t         viewangle;

int32_t         depth;
_prmirror       mirrors[10];

#if defined __clang__ && defined __APPLE__
// XXX: OS X 10.9 deprecated GLUtesselator.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
GLUtesselator*  prtess;
#if defined __clang__ && defined __APPLE__
#pragma clang diagnostic pop
#endif

static int16_t  cursky;
static char     curskypal;
static int8_t   curskyshade;
static float    curskyangmul = 1;

_pranimatespritesinfo asi;

int32_t         polymersearching;

int32_t         culledface;

// EXTERNAL FUNCTIONS
int32_t             polymer_init(void)
{
    int32_t         i, j, t = getticks();

    if (pr_verbosity >= 1) OSD_Printf("Initializing Polymer subsystem...\n");

    if (!glinfo.texnpot ||
        !glinfo.depthtex ||
        !glinfo.shadow ||
        !glinfo.fbos ||
        !glinfo.rect ||
        !glinfo.multitex ||
        !glinfo.vbos ||
        !glinfo.occlusionqueries ||
        !glinfo.glsl)
    {
        OSD_Printf("PR : Your video card driver/combo doesn't support the necessary features!\n");
        OSD_Printf("PR : Disabling Polymer...\n");
        return 0;
    }

    // clean up existing stuff since it will be initialized again if we're re-entering here
    polymer_uninit();

    Bmemset(&prsectors[0], 0, sizeof(prsectors[0]) * MAXSECTORS);
    Bmemset(&prwalls[0], 0, sizeof(prwalls[0]) * MAXWALLS);

    prtess = bgluNewTess();
    if (prtess == 0)
    {
        OSD_Printf("PR : Tessellation object initialization failed!\n");
        return 0;
    }

    polymer_loadboard();

    polymer_initartsky();
    skyboxdatavbo = 0;

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

    polymersearching = FALSE;

    polymer_initrendertargets(pr_shadowcount + 1);

    // Prime highpalookup maps
    i = 0;
    while (i < MAXBASEPALS)
    {
        j = 0;
        while (j < MAXPALOOKUPS)
        {
            if (prhighpalookups[i][j].data)
            {
                bglGenTextures(1, &prhighpalookups[i][j].map);
                bglBindTexture(GL_TEXTURE_3D, prhighpalookups[i][j].map);
                bglTexImage3D(GL_TEXTURE_3D,                // target
                              0,                            // mip level
                              GL_RGBA,                      // internalFormat
                              PR_HIGHPALOOKUP_DIM,          // width
                              PR_HIGHPALOOKUP_DIM,          // height
                              PR_HIGHPALOOKUP_DIM,          // depth
                              0,                            // border
                              GL_BGRA,                      // upload format
                              GL_UNSIGNED_BYTE,             // upload component type
                              prhighpalookups[i][j].data);     // data pointer
                bglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                bglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                bglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
                bglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
                bglTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
                bglBindTexture(GL_TEXTURE_3D, 0);
            }
            j++;
        }
        i++;
    }

#ifndef __APPLE__
    if (glinfo.debugoutput) {
        // Enable everything.
        bglDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        bglDebugMessageCallbackARB((GLDEBUGPROCARB)polymer_debugoutputcallback, NULL);
        bglEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }
#endif

    if (pr_verbosity >= 1) OSD_Printf("PR : Initialization complete in %d ms.\n", getticks()-t);

    return 1;
}

void                polymer_uninit(void)
{
    int32_t         i, j;

    if (prtess)
    {
        bgluDeleteTess(prtess);
        prtess = NULL;
    }

    polymer_freeboard();

    polymer_initrendertargets(0);

    i = 0;
    while (i < MAXBASEPALS)
    {
        j = 0;
        while (j < MAXPALOOKUPS)
        {
//            if (prhighpalookups[i][j].data) {
//                DO_FREE_AND_NULL(prhighpalookups[i][j].data);
//            }
            if (prhighpalookups[i][j].map) {
                bglDeleteTextures(1, &prhighpalookups[i][j].map);
                prhighpalookups[i][j].map = 0;
            }
            j++;
        }
        i++;
    }

    i = 0;
    while (plpool)
    {
        _prplanelist*   next = plpool->n;

        Bfree(plpool);
        plpool = next;
        i++;
    }

    if (pr_verbosity >= 3)
        OSD_Printf("PR: freed %d planelists\n", i);
}

void                polymer_setaspect(int32_t ang)
{
    float           aspect;
    float fang = (float)ang * atanf(fviewingrange*(1.f/65536.f)) * (4.f/fPI);

    if (pr_customaspect != 0.0f)
        aspect = pr_customaspect;
    else
        aspect = (float)(windowxy2.x-windowxy1.x+1) /
                 (float)(windowxy2.y-windowxy1.y+1);

    bglMatrixMode(GL_PROJECTION);
    bglLoadIdentity();
    bgluPerspective(fang * (360.f/2048.f), aspect, 0.01f, 100.0f);
}

void                polymer_glinit(void)
{
    bglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    bglClearStencil(0);
    bglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    bglViewport(windowxy1.x, yres-(windowxy2.y+1),windowxy2.x-windowxy1.x+1, windowxy2.y-windowxy1.y+1);

    // texturing
    bglEnable(GL_TEXTURE_2D);

    bglEnable(GL_DEPTH_TEST);
    bglDepthFunc(GL_LEQUAL);

    bglDisable(GL_BLEND);
    bglDisable(GL_ALPHA_TEST);

    if (pr_wireframe)
        bglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        bglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    polymer_setaspect(pr_fov);

    bglMatrixMode(GL_MODELVIEW);
    bglLoadIdentity();

    bglEnableClientState(GL_VERTEX_ARRAY);
    bglEnableClientState(GL_TEXTURE_COORD_ARRAY);

    bglDisable(GL_FOG);

    culledface = GL_BACK;
    bglCullFace(GL_BACK);
    bglFrontFace(GL_CCW);

    bglEnable(GL_CULL_FACE);
}

void                polymer_resetlights(void)
{
    int32_t         i;
    _prsector       *s;
    _prwall         *w;

    i = 0;
    while (i < numsectors)
    {
        s = prsectors[i];

        if (!s) {
            i++;
            continue;
        }

        polymer_resetplanelights(&s->floor);
        polymer_resetplanelights(&s->ceil);

        i++;
    }

    i = 0;
    while (i < numwalls)
    {
        w = prwalls[i];

        if (!w) {
            i++;
            continue;
        }

        polymer_resetplanelights(&w->wall);
        polymer_resetplanelights(&w->over);
        polymer_resetplanelights(&w->mask);

        i++;
    }

    i = 0;
    while (i < PR_MAXLIGHTS)
    {
        prlights[i].flags.active = 0;
        i++;
    }

    lightcount = 0;

    if (!loadmaphack(NULL))
        OSD_Printf("polymer_resetlights: reloaded maphack\n");
}

void                polymer_loadboard(void)
{
    int32_t         i;

    polymer_freeboard();

    // in the big map buffer, sectors have floor and ceiling vertices for each wall first, then walls
    prwalldataoffset = numwalls * 2 * sizeof(_prvert);

    bglGenBuffersARB(1, &prmapvbo);
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, prwalldataoffset + (numwalls * prwalldatasize), NULL, mapvbousage);
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    bglGenBuffersARB(1, &prindexringvbo);
    bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, prindexringvbo);

    if (pr_buckets)
    {
        bglBufferStorage(GL_ELEMENT_ARRAY_BUFFER, prindexringsize * sizeof(GLuint), NULL, prindexringmapflags | GL_DYNAMIC_STORAGE_BIT);
        prindexring = (GLuint*)bglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, prindexringsize * sizeof(GLuint), prindexringmapflags);
    }

    bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);

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

    polymer_resetlights();

    if (pr_verbosity >= 1 && numsectors) OSD_Printf("PR : Board loaded.\n");
}

// The parallaxed ART sky angle divisor corresponding to a horizfrac of 32768.
#define DEFAULT_ARTSKY_ANGDIV 4.3027f

void                polymer_drawrooms(int32_t daposx, int32_t daposy, int32_t daposz, int16_t daang, int32_t dahoriz, int16_t dacursectnum)
{
    int16_t         cursectnum;
    int32_t         i, cursectflorz, cursectceilz;
    float           skyhoriz, ang, tiltang;
    float           pos[3];
    pthtyp*         pth;

    if (getrendermode() == REND_CLASSIC) return;

    begindrawing();

    // TODO: support for screen resizing
    // frameoffset = frameplace + windowxy1.y*bytesperline + windowxy1.x;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing rooms...\n");

    // fogcalc_old needs this
    gvisibility = ((float)globalvisibility)*FOGSCALE;

    ang = (float)(daang) * (360.f/2048.f);
    horizang = (float)(-getangle(128, dahoriz-100)) * (360.f/2048.f);
    tiltang = (gtang * 90.0f);

    pos[0] = (float)daposy;
    pos[1] = -(float)(daposz) / 16.0f;
    pos[2] = -(float)daposx;

    polymer_updatelights();

//     polymer_resetlights();
//     if (pr_lighting)
//         polymer_applylights();

    depth = 0;

    if (pr_shadows && lightcount && (pr_shadowcount > 0))
        polymer_prepareshadows();

    // hack for parallax skies
    skyhoriz = horizang;
    if (skyhoriz < -180.0f)
        skyhoriz += 360.0f;

    drawingskybox = 1;
    pth = texcache_fetch(cursky, 0, 0, DAMETH_NOMASK);
    drawingskybox = 0;

    // if it's not a skybox, make the sky parallax
    // DEFAULT_ARTSKY_ANGDIV is computed from eyeballed values
    // need to recompute it if we ever change the max horiz amplitude
    if (!pth || !(pth->flags & PTH_SKYBOX))
        skyhoriz *= curskyangmul;

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
    updatesectorbreadth(daposx, daposy, &cursectnum);

    if (cursectnum >= 0 && cursectnum < numsectors)
        dacursectnum = cursectnum;
    else if (pr_verbosity>=2)
        OSD_Printf("PR : got sector %d after update!\n", cursectnum);

    // unflag all sectors
    i = numsectors-1;
    while (i >= 0)
    {
        prsectors[i]->flags.uptodate = 0;
        prsectors[i]->wallsproffset = 0.0f;
        prsectors[i]->floorsproffset = 0.0f;
        i--;
    }
    i = numwalls-1;
    while (i >= 0)
    {
        prwalls[i]->flags.uptodate = 0;
        i--;
    }

    if (searchit == 2 && !polymersearching)
    {
        globaloldoverridematerial = overridematerial;
        overridematerial = prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit;
        overridematerial |= prprogrambits[PR_BIT_DIFFUSE_MAP2].bit;
        polymersearching = TRUE;
    }
    if (!searchit && polymersearching) {
        overridematerial = globaloldoverridematerial;
        polymersearching = FALSE;
    }

    if (dacursectnum > -1 && dacursectnum < numsectors)
        getzsofslope(dacursectnum, daposx, daposy, &cursectceilz, &cursectflorz);

    // external view (editor)
    if ((dacursectnum < 0) || (dacursectnum >= numsectors) ||
            (daposz > cursectflorz) ||
            (daposz < cursectceilz))
    {
        prcanbucket = 1;

        if (!editstatus && pr_verbosity>=1)
        {
            if ((unsigned)dacursectnum < (unsigned)numsectors)
                OSD_Printf("PR : EXT sec=%d  z=%d (%d, %d)\n", dacursectnum, daposz, cursectflorz, cursectceilz);
            else
                OSD_Printf("PR : EXT sec=%d  z=%d\n", dacursectnum, daposz);
        }

        curmodelviewmatrix = rootmodelviewmatrix;
        i = numsectors-1;
        while (i >= 0)
        {
            polymer_updatesector(i);
            polymer_drawsector(i, FALSE);
            polymer_scansprites(i, tsprite, &spritesortcnt);
            i--;
        }

        i = numwalls-1;
        while (i >= 0)
        {
            polymer_updatewall(i);
            polymer_drawwall(sectorofwall(i), i);
            i--;
        }

        polymer_emptybuckets();

        viewangle = daang;
        enddrawing();
        return;
    }

    // GO!
    polymer_displayrooms(dacursectnum);

    curmodelviewmatrix = rootmodelviewmatrix;

    // build globals used by rotatesprite
    viewangle = daang;
    set_globalang(daang);

    // polymost globals used by polymost_dorotatesprite
    gcosang = fcosglobalang*(1./262144.f);
    gsinang = fsinglobalang*(1./262144.f);
    gcosang2 = gcosang*fviewingrange*(1./65536.f);
    gsinang2 = gsinang*fviewingrange*(1./65536.f);

    if (pr_verbosity >= 3) OSD_Printf("PR : Rooms drawn.\n");
    enddrawing();
}

void                polymer_drawmasks(void)
{
    bglEnable(GL_ALPHA_TEST);
    bglEnable(GL_BLEND);
//     bglEnable(GL_POLYGON_OFFSET_FILL);

//     while (--spritesortcnt)
//     {
//         tspriteptr[spritesortcnt] = &tsprite[spritesortcnt];
//         polymer_drawsprite(spritesortcnt);
//     }

    bglEnable(GL_CULL_FACE);

    if (cursectormaskcount) {
        // We (kind of) queue sector masks near to far, so drawing them in reverse
        // order is the sane approach here. Of course impossible cases will arise.
        while (*cursectormaskcount) {
            polymer_drawsector(cursectormasks[--(*cursectormaskcount)], TRUE);
        }

        // This should _always_ be called after a corresponding pr_displayrooms()
        // unless we're in "external view" mode, which was checked above.
        // Both the top-level game drawrooms and the recursive internal passes
        // should be accounted for here. If these free cause corruption, there's
        // an accounting bug somewhere.
        DO_FREE_AND_NULL(cursectormaskcount);
        DO_FREE_AND_NULL(cursectormasks);
    }

    bglDisable(GL_CULL_FACE);

//     bglDisable(GL_POLYGON_OFFSET_FILL);
    bglDisable(GL_BLEND);
    bglDisable(GL_ALPHA_TEST);
}

void                polymer_editorpick(void)
{
    GLubyte         picked[3];
    int16_t         num;

    bglReadPixels(searchx, ydim - searchy, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, picked);

    num = B_UNBUF16(&picked[1]);

    searchstat = picked[0];

    switch (searchstat) {
    case 0: // wall
    case 5: // botomwall
    case 4: // 1-way/masked wall
        searchsector = sectorofwall(num);
        searchbottomwall = searchwall = num;
        searchisbottom = (searchstat==5);
        if (searchstat == 5) {
            searchstat = 0;
            if (wall[num].nextwall >= 0 && (wall[num].cstat & 2)) {
                searchbottomwall = wall[num].nextwall;
            }
        }
        break;
    case 1: // floor
    case 2: // ceiling
        searchsector = num;

        // Apologies to Plagman for littering here, but this feature is quite essential
        {
            GLdouble model[16];
            GLdouble proj[16];
            GLint view[4];

            GLdouble x,y,z;
            GLfloat scr[3], scrv[3];
            GLdouble scrx,scry,scrz;
            GLfloat dadepth;

            int16_t k, bestk=0;
            GLfloat bestwdistsq = (GLfloat)3.4e38, wdistsq;
            GLfloat w1[2], w2[2], w21[2], pw1[2], pw2[2];
            GLfloat ptonline[2];
            GLfloat scrvxz[2];
            GLfloat scrvxznorm, scrvxzn[2], scrpxz[2];
            GLfloat w1d, w2d;
            walltype *wal = &wall[sector[searchsector].wallptr];

            GLfloat t, svcoeff, p[2];
            GLfloat *pl;

            bglGetDoublev(GL_MODELVIEW_MATRIX, model);
            bglGetDoublev(GL_PROJECTION_MATRIX, proj);
            bglGetIntegerv(GL_VIEWPORT, view);

            bglReadPixels(searchx, ydim-searchy, 1,1, GL_DEPTH_COMPONENT, GL_FLOAT, &dadepth);
            bgluUnProject(searchx, ydim-searchy, dadepth,  model, proj, view,  &x, &y, &z);
            bgluUnProject(searchx, ydim-searchy, 0.0,  model, proj, view,  &scrx, &scry, &scrz);

            scr[0]=scrx, scr[1]=scry, scr[2]=scrz;

            scrv[0] = x-scrx;
            scrv[1] = y-scry;
            scrv[2] = z-scrz;

            scrvxz[0] = x-scrx;
            scrvxz[1] = z-scrz;

            if (prsectors[searchsector]==NULL)
            {
                //OSD_Printf("polymer_editorpick: prsectors[searchsector]==NULL !!!\n");
                searchwall = sector[num].wallptr;
            }
            else
            {
                if (searchstat==1)
                    pl = prsectors[searchsector]->ceil.plane;
                else
                    pl = prsectors[searchsector]->floor.plane;

                if (pl == NULL)
                {
                    searchwall = sector[num].wallptr;
                    return;
                }

                t = dot3f(pl,scrv);
                svcoeff = -(dot3f(pl,scr)+pl[3])/t;

                // point on plane (x and z)
                p[0] = scrx + svcoeff*scrv[0];
                p[1] = scrz + svcoeff*scrv[2];

                for (k=0; k<sector[searchsector].wallnum; k++)
                {
                    w1[1] = -(float)wal[k].x;
                    w1[0] = (float)wal[k].y;
                    w2[1] = -(float)wall[wal[k].point2].x;
                    w2[0] = (float)wall[wal[k].point2].y;

                    scrvxznorm = sqrt(dot2f(scrvxz,scrvxz));
                    scrvxzn[0] = scrvxz[1]/scrvxznorm;
                    scrvxzn[1] = -scrvxz[0]/scrvxznorm;

                    relvec2f(p,w1, pw1);
                    relvec2f(p,w2, pw2);
                    relvec2f(w2,w1, w21);

                    w1d = dot2f(scrvxzn,pw1);
                    w2d = dot2f(scrvxzn,pw2);
                    w2d = -w2d;
                    if (w1d <= 0 || w2d <= 0)
                        continue;

                    ptonline[0] = w2[0]+(w2d/(w1d+w2d))*w21[0];
                    ptonline[1] = w2[1]+(w2d/(w1d+w2d))*w21[1];
                    relvec2f(p,ptonline, scrpxz);
                    if (dot2f(scrvxz,scrpxz)<0)
                        continue;

                    wdistsq = dot2f(scrpxz,scrpxz);
                    if (wdistsq < bestwdistsq)
                    {
                        bestk = k;
                        bestwdistsq = wdistsq;
                    }
                }

                searchwall = sector[searchsector].wallptr + bestk;
            }
        }
        // :P

//        searchwall = sector[num].wallptr;
        break;
    case 3:
        // sprite
        searchsector = sprite[num].sectnum;
        searchwall = num;
        break;
    }

    searchit = 0;
}

void                polymer_inb4rotatesprite(int16_t tilenum, char pal, int8_t shade, int32_t method)
{
    _prmaterial     rotatespritematerial;

    polymer_getbuildmaterial(&rotatespritematerial, tilenum, pal, shade, 0, method);

    rotatespritematerialbits = polymer_bindmaterial(&rotatespritematerial, NULL, 0);
}

void                polymer_postrotatesprite(void)
{
    polymer_unbindmaterial(rotatespritematerialbits);
}

static void         polymer_setupdiffusemodulation(_prplane *plane, GLubyte modulation, GLubyte *data)
{
    plane->material.diffusemodulation[0] = modulation;
    plane->material.diffusemodulation[1] = ((GLubyte *) data)[0];
    plane->material.diffusemodulation[2] = ((GLubyte *) data)[1];
    plane->material.diffusemodulation[3] = 0xFF;
}

static void         polymer_drawsearchplane(_prplane *plane, GLubyte *oldcolor, GLubyte modulation, GLubyte *data)
{
    Bmemcpy(oldcolor, plane->material.diffusemodulation, sizeof(GLubyte) * 4);

    polymer_setupdiffusemodulation(plane, modulation, data);

    polymer_drawplane(plane);

    Bmemcpy(plane->material.diffusemodulation, oldcolor, sizeof(GLubyte) * 4);
}

void                polymer_drawmaskwall(int32_t damaskwallcnt)
{
    usectortype     *sec;
    walltype        *wal;
    _prwall         *w;
    GLubyte         oldcolor[4];

    if (pr_verbosity >= 3) OSD_Printf("PR : Masked wall %i...\n", damaskwallcnt);

    sec = (usectortype *)&sector[sectorofwall(maskwall[damaskwallcnt])];
    wal = &wall[maskwall[damaskwallcnt]];
    w = prwalls[maskwall[damaskwallcnt]];

    bglEnable(GL_CULL_FACE);

    if (searchit == 2) {
        polymer_drawsearchplane(&w->mask, oldcolor, 0x04, (GLubyte *)&maskwall[damaskwallcnt]);
    } else {
        calc_and_apply_fog(wal->picnum, fogshade(wal->shade, wal->pal), sec->visibility, get_floor_fogpal(sec));
        polymer_drawplane(&w->mask);
    }

    bglDisable(GL_CULL_FACE);
}

void                polymer_drawsprite(int32_t snum)
{
    int32_t         i, j, cs;
    _prsprite       *s;

    uspritetype      *const tspr = tspriteptr[snum];
    const usectortype *sec;

    if (pr_verbosity >= 3) OSD_Printf("PR : Sprite %i...\n", snum);

    if (bad_tspr(tspr))
        return;

    if ((tspr->cstat & 8192) && (depth && !mirrors[depth-1].plane))
        return;

    if ((tspr->cstat & 16384) && (!depth || mirrors[depth-1].plane))
        return;

    DO_TILE_ANIM(tspr->picnum, tspr->owner+32768);

    sec = (usectortype *)&sector[tspr->sectnum];
    calc_and_apply_fog(tspr->picnum, fogshade(tspr->shade, tspr->pal), sec->visibility,
                       get_floor_fogpal((usectortype *)&sector[tspr->sectnum]));

    if (usemodels && tile2model[Ptile2tile(tspr->picnum,tspr->pal)].modelid >= 0 &&
        tile2model[Ptile2tile(tspr->picnum,tspr->pal)].framenum >= 0 &&
        !(spriteext[tspr->owner].flags & SPREXT_NOTMD))
    {
        bglEnable(GL_CULL_FACE);
        SWITCH_CULL_DIRECTION;
        polymer_drawmdsprite(tspr);
        SWITCH_CULL_DIRECTION;
        bglDisable(GL_CULL_FACE);
        return;
    }

    cs = tspr->cstat;

    // I think messing with the tspr is safe at this point?
    // If not, change that to modify a temp position in updatesprite itself.
    // I don't think this flags are meant to change on the fly so it'd possibly
    // be safe to cache a plane that has them applied.
    if (spriteext[tspr->owner].flags & SPREXT_AWAY1)
    {
        tspr->x += sintable[(tspr->ang + 512) & 2047] >> 13;
        tspr->y += sintable[tspr->ang & 2047] >> 13;
    }
    else if (spriteext[tspr->owner].flags & SPREXT_AWAY2)
    {
        tspr->x -= sintable[(tspr->ang + 512) & 2047] >> 13;
        tspr->y -= sintable[tspr->ang & 2047] >> 13;
    }

    polymer_updatesprite(snum);

    Bassert(tspr->owner < MAXSPRITES);
    s = prsprites[tspr->owner];

    if (s == NULL)
        return;

    switch ((tspr->cstat>>4) & 3)
    {
    case 1:
        prsectors[tspr->sectnum]->wallsproffset += 0.5f;
        if (!depth || mirrors[depth-1].plane)
            bglPolygonOffset(-1.0f, -1.0f);
        break;
    case 2:
        prsectors[tspr->sectnum]->floorsproffset += 0.5f;
        if (!depth || mirrors[depth-1].plane)
            bglPolygonOffset(-1.0f, -1.0f);
        break;
    }

    if ((cs & 48) == 0)
    {
        int32_t curpriority = 0;

        s->plane.lightcount = 0;

        while ((curpriority < pr_maxlightpriority) && (!depth || mirrors[depth-1].plane))
        {
            i = j = 0;
            while (j < lightcount)
            {
                while (!prlights[i].flags.active)
                    i++;

                if (prlights[i].priority != curpriority)
                {
                    i++;
                    j++;
                    continue;
                }

                if (polymer_planeinlight(&s->plane, &prlights[i]))
                    s->plane.lights[s->plane.lightcount++] = i;

                i++;
                j++;
            }
            curpriority++;
        }
    }

    if ((tspr->cstat & 64) && (tspr->cstat & SPR_ALIGN_MASK))
    {
        if ((tspr->cstat & SPR_ALIGN_MASK)==SPR_FLOOR && (tspr->cstat & SPR_YFLIP))
            SWITCH_CULL_DIRECTION;
        bglEnable(GL_CULL_FACE);
    }

    if ((!depth || mirrors[depth-1].plane) && !pr_ati_nodepthoffset)
        bglEnable(GL_POLYGON_OFFSET_FILL);

    polymer_drawplane(&s->plane);

    if ((!depth || mirrors[depth-1].plane) && !pr_ati_nodepthoffset)
        bglDisable(GL_POLYGON_OFFSET_FILL);

    if ((tspr->cstat & 64) && (tspr->cstat & SPR_ALIGN_MASK))
    {
        if ((tspr->cstat & SPR_ALIGN_MASK)==SPR_FLOOR && (tspr->cstat & SPR_YFLIP))
            SWITCH_CULL_DIRECTION;
        bglDisable(GL_CULL_FACE);
    }
}

void                polymer_setanimatesprites(animatespritesptr animatesprites, int32_t x, int32_t y, int32_t a, int32_t smoothratio)
{
    asi.animatesprites = animatesprites;
    asi.x = x;
    asi.y = y;
    asi.a = a;
    asi.smoothratio = smoothratio;
}

int16_t             polymer_addlight(_prlight* light)
{
    int32_t         lighti;

    if (lightcount >= PR_MAXLIGHTS || light->priority > pr_maxlightpriority || !pr_lighting)
        return -1;

    if ((light->sector == -1) || (light->sector >= numsectors))
        return -1;

    lighti = 0;
    while ((lighti < PR_MAXLIGHTS) && (prlights[lighti].flags.active))
        lighti++;

    if (lighti == PR_MAXLIGHTS)
        return -1;
#if 0
    // Spot lights disabled on ATI cards because they cause crashes with
    // Catalyst 12.8 drivers.
    // See: http://forums.duke4.net/topic/5723-hrp-polymer-crash/
    if (pr_ati_fboworkaround && light->radius)
        return -1;
#endif
    Bmemcpy(&prlights[lighti], light, sizeof(_prlight));

    if (light->radius) {
        polymer_processspotlight(&prlights[lighti]);

        // get the texture handle for the lightmap
        if (light->tilenum > 0) {
            int16_t     picnum = light->tilenum;
            pthtyp*     pth;

            DO_TILE_ANIM(picnum, 0);

            if (!waloff[picnum])
                loadtile(picnum);

            pth = NULL;
            pth = texcache_fetch(picnum, 0, 0, DAMETH_NOMASK);

            if (pth)
                light->lightmap = pth->glpic;
        }
    }

    prlights[lighti].flags.isinview = 0;
    prlights[lighti].flags.active = 1;

    prlights[lighti].flags.invalidate = 0;

    prlights[lighti].planecount = 0;
    prlights[lighti].planelist = NULL;

    polymer_culllight(lighti);

    lightcount++;

    return lighti;
}

void                polymer_deletelight(int16_t lighti)
{
    if (!prlights[lighti].flags.active)
    {
#ifdef DEBUGGINGAIDS
        if (pr_verbosity >= 2)
            OSD_Printf("PR : Called polymer_deletelight on inactive light\n");
        // currently known cases: when reloading maphack lights (didn't set maphacklightcnt=0
        // but did loadmaphack()->delete_maphack_lights() after polymer_resetlights())
#endif
        return;
    }

    polymer_removelight(lighti);

    prlights[lighti].flags.active = 0;

    lightcount--;
}

void                polymer_invalidatelights(void)
{
    int32_t         i = PR_MAXLIGHTS-1;

    do
        prlights[i].flags.invalidate = prlights[i].flags.active;
    while (i--);
}

void                polymer_texinvalidate(void)
{
    int32_t         i;

    i = 0;

    while (i < MAXSPRITES) {
        polymer_invalidatesprite(i);
        i++;
    }

    i = numsectors - 1;

    if (!numsectors || !prsectors[i])
        return;

    do
        prsectors[i--]->flags.invalidtex = 1;
    while (i >= 0);

    i = numwalls - 1;
    do
        prwalls[i--]->flags.invalidtex = 1;
    while (i >= 0);
}

void                polymer_definehighpalookup(char basepalnum, char palnum, char *data)
{
    prhighpalookups[basepalnum][palnum].data = (char *)Xmalloc(PR_HIGHPALOOKUP_DATA_SIZE);

    Bmemcpy(prhighpalookups[basepalnum][palnum].data, data, PR_HIGHPALOOKUP_DATA_SIZE);
}

int32_t             polymer_havehighpalookup(int32_t basepalnum, int32_t palnum)
{
    if ((uint32_t)basepalnum >= MAXBASEPALS || (uint32_t)palnum >= MAXPALOOKUPS)
        return 0;

    return (prhighpalookups[basepalnum][palnum].data != NULL);
}


// CORE
static void         polymer_displayrooms(const int16_t dacursectnum)
{
    usectortype      *sec;
    int32_t         i;
    int16_t         bunchnum;
    int16_t         ns;
    GLint           result;
    int16_t         doquery;
    int32_t         front;
    int32_t         back;
    GLfloat         localskymodelviewmatrix[16];
    GLfloat         localmodelviewmatrix[16];
    GLfloat         localprojectionmatrix[16];
    float           frustum[5 * 4];
    int32_t         localspritesortcnt;
    uspritetype     localtsprite[MAXSPRITESONSCREEN];
    int16_t         localmaskwall[MAXWALLSB];
    int16_t         localmaskwallcnt;
    _prmirror       mirrorlist[10];
    int             mirrorcount;
    int16_t         *localsectormasks;
    int16_t         *localsectormaskcount;
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

    localsectormasks = (int16_t *)Xmalloc(sizeof(int16_t) * numsectors);
    localsectormaskcount = (int16_t *)Xcalloc(sizeof(int16_t), 1);
    cursectormasks = localsectormasks;
    cursectormaskcount = localsectormaskcount;

    bglDisable(GL_DEPTH_TEST);
    bglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    polymer_drawsky(cursky, curskypal, curskyshade);
    bglEnable(GL_DEPTH_TEST);

    // depth-only occlusion testing pass
//     overridematerial = 0;

    prcanbucket = 1;

    while (front != back)
    {
        sec = (usectortype *)&sector[sectorqueue[front]];

        polymer_pokesector(sectorqueue[front]);
        polymer_drawsector(sectorqueue[front], FALSE);
        polymer_scansprites(sectorqueue[front], localtsprite, &localspritesortcnt);

        doquery = 0;

        i = sec->wallnum-1;
        do
        {
            // if we have a level boundary somewhere in the sector,
            // consider these walls as visportals
            if (wall[sec->wallptr + i].nextsector < 0 && pr_buckets == 0)
                doquery = 1;
        }
        while (--i >= 0);

        i = sec->wallnum-1;
        while (i >= 0)
        {
            if ((wall[sec->wallptr + i].nextsector >= 0) &&
                (wallvisible(globalposx, globalposy, sec->wallptr + i)) &&
                (polymer_planeinfrustum(&prwalls[sec->wallptr + i]->mask, frustum)))
            {
                if ((prwalls[sec->wallptr + i]->mask.vertcount == 4) &&
                    !(prwalls[sec->wallptr + i]->underover & 4) &&
                    !(prwalls[sec->wallptr + i]->underover & 8))
                {
                    // early exit for closed sectors
                    _prwall         *w;

                    w = prwalls[sec->wallptr + i];

                    if ((w->mask.buffer[0].y >= w->mask.buffer[3].y) &&
                        (w->mask.buffer[1].y >= w->mask.buffer[2].y))
                    {
                        i--;
                        continue;
                    }
                }

                if ((wall[sec->wallptr + i].cstat & 48) == 16)
                {
                    int pic = wall[sec->wallptr + i].overpicnum;

                    if (tilesiz[pic].x > 0 && tilesiz[pic].y > 0)
                        localmaskwall[localmaskwallcnt++] = sec->wallptr + i;
                }

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

                        pos[0] = fglobalposy;
                        pos[1] = fglobalposz * (-1.f/16.f);
                        pos[2] = -fglobalposx;

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
                        queryid[sec->wallptr + i] = 0xFFFFFFFF;
                }
            }

            i--;
        }

        // Cram as much CPU or GPU work as we can between queuing the
        // occlusion queries and reaping them.
        i = sec->wallnum-1;
        do
        {
            if (wallvisible(globalposx, globalposy, sec->wallptr + i))
                polymer_drawwall(sectorqueue[front], sec->wallptr + i);
        }
        while (--i >= 0);
#ifdef YAX_ENABLE
        // queue ROR neighbors
        if ((bunchnum = yax_getbunch(sectorqueue[front], YAX_FLOOR)) >= 0) {

            for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, ns)) {

                if (ns >= 0 && !drawingstate[ns] &&
                    polymer_planeinfrustum(&prsectors[ns]->ceil, frustum)) {

                    sectorqueue[back++] = ns;
                    drawingstate[ns] = 1;
                }
            }
        }

        if ((bunchnum = yax_getbunch(sectorqueue[front], YAX_CEILING)) >= 0) {

            for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, ns)) {

                if (ns >= 0 && !drawingstate[ns] &&
                    polymer_planeinfrustum(&prsectors[ns]->floor, frustum)) {

                    sectorqueue[back++] = ns;
                    drawingstate[ns] = 1;
                }
            }
        }
#endif
        i = sec->wallnum-1;
        do
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
            } else if (queryid[sec->wallptr + i] &&
                       queryid[sec->wallptr + i] != 0xFFFFFFFF)
            {
                bglDeleteQueriesARB(1, &queryid[sec->wallptr + i]);
                queryid[sec->wallptr + i] = 0;
            }
        }
        while (--i >= 0);

        front++;
    }

    polymer_emptybuckets();

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

    i = mirrorcount-1;
    while (i >= 0)
    {
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, prrts[0].fbo);
        bglPushAttrib(GL_VIEWPORT_BIT);
        bglViewport(windowxy1.x, yres-(windowxy2.y+1),windowxy2.x-windowxy1.x+1, windowxy2.y-windowxy1.y+1);

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
        SWITCH_CULL_DIRECTION;
        //bglEnable(GL_CLIP_PLANE0);

        if (mirrorlist[i].wallnum >= 0)
            preparemirror(globalposx, globalposy, globalang,
                          mirrorlist[i].wallnum, &gx, &gy, &viewangle);

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

        px = (int32_t)(-coeff*mirrorlist[i].plane->plane[0]*2 + px);
        py = (int32_t)(-coeff*mirrorlist[i].plane->plane[1]*2 + py);
        pz = (int32_t)(-coeff*mirrorlist[i].plane->plane[2]*2 + pz);

        // map back from polymer to build
        set_globalpos(-pz, px, -py * 16);

        mirrors[depth++] = mirrorlist[i];
        polymer_displayrooms(mirrorlist[i].sectnum);
        depth--;

        cursectormasks = localsectormasks;
        cursectormaskcount = localsectormaskcount;

        set_globalpos(gx, gy, gz);

        bglDisable(GL_CLIP_PLANE0);
        SWITCH_CULL_DIRECTION;
        bglMatrixMode(GL_MODELVIEW);
        bglPopMatrix();

        bglPopAttrib();
        bglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        mirrorlist[i].plane->material.mirrormap = prrts[0].color;
        polymer_drawplane(mirrorlist[i].plane);
        mirrorlist[i].plane->material.mirrormap = 0;

        i--;
    }

    spritesortcnt = localspritesortcnt;
    Bmemcpy(tsprite, localtsprite, sizeof(spritetype) * spritesortcnt);
    maskwallcnt = localmaskwallcnt;
    Bmemcpy(maskwall, localmaskwall, sizeof(int16_t) * maskwallcnt);

    if (depth)
    {
        set_globalang(viewangle);

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

static void         polymer_emptybuckets(void)
{
    _prbucket *bucket = prbuckethead;

    if (pr_buckets == 0)
        return;

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
    bglVertexPointer(3, GL_FLOAT, sizeof(_prvert), NULL);
    bglTexCoordPointer(2, GL_FLOAT, sizeof(_prvert), (GLvoid *)(3 * sizeof(GLfloat)));

    bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, prindexringvbo);

    uint32_t indexcount = 0;
    while (bucket != NULL)
    {
        indexcount += bucket->count;

        bucket = bucket->next;
    }

    // ensure space in index ring, wrap otherwise
    if (indexcount + prindexringoffset >= (unsigned)prindexringsize)
    {
        bglUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER);
        prindexring = (GLuint *)bglMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, prindexringsize * sizeof(GLuint), GL_MAP_INVALIDATE_BUFFER_BIT | prindexringmapflags);
        prindexringoffset = 0;
    }

    // put indices in the ring, all at once
    bucket = prbuckethead;

    while (bucket != NULL)
    {
        if (bucket->count == 0)
        {
            bucket = bucket->next;
            continue;
        }

        memcpy(&prindexring[prindexringoffset], bucket->indices, bucket->count * sizeof(GLuint));

        bucket->indiceoffset = (GLuint*)(prindexringoffset * sizeof(GLuint));

        prindexringoffset += bucket->count;

        bucket = bucket->next;
    }

    bucket = prbuckethead;

    while (bucket != NULL)
    {
        if (bucket->count == 0)
        {
            bucket = bucket->next;
            continue;
        }

        int32_t materialbits = polymer_bindmaterial(&bucket->material, NULL, 0);

        bglDrawElements(GL_TRIANGLES, bucket->count, GL_UNSIGNED_INT, bucket->indiceoffset);

        polymer_unbindmaterial(materialbits);

        bucket->count = 0;

        bucket = bucket->next;
    }

    bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    prcanbucket = 0;
}

static hashtable_t h_buckets      = { 2048, NULL };

static _prbucket*   polymer_findbucket(int16_t tilenum, char pal)
{
    char propstr[16];

    Bsprintf(propstr, "%d_%d", tilenum, pal);

    _prbucket *bucketptr = prbuckethead ? (_prbucket *)hash_find(&h_buckets, propstr) : NULL;

    // find bucket

    // no buckets or no bucket found, create one
    if (bucketptr == NULL || (intptr_t)bucketptr == -1)
    {
        bucketptr = (_prbucket *)Xmalloc(sizeof (_prbucket));

        if (h_buckets.items == NULL)
            hash_init(&h_buckets);

        // insert, since most likely to use same pattern next frame
        // will need to reorder by MRU first every once in a while
        // or move to hashing lookup
        bucketptr->next = prbuckethead;
        prbuckethead = bucketptr;

        bucketptr->tilenum = tilenum;
        bucketptr->pal = pal;

        bucketptr->invalidmaterial = 1;

        bucketptr->count = 0;
        bucketptr->buffersize = 1024;
        bucketptr->indices = (GLuint *)Xmalloc(bucketptr->buffersize * sizeof(GLuint));

        hash_add(&h_buckets, propstr, (intptr_t)bucketptr, 1);
    }

    return bucketptr;
}

static void         polymer_bucketplane(_prplane* plane)
{
    _prbucket *bucketptr = plane->bucket;
    uint32_t neededindicecount;
    int32_t i;

    // we don't keep buffers for quads
    neededindicecount = (plane->indicescount == 0) ? 6 : plane->indicescount;

    // ensure size
    while (bucketptr->count + neededindicecount >= bucketptr->buffersize)
    {
        bucketptr->buffersize *= 2;
        bucketptr->indices = (GLuint *)Xrealloc(bucketptr->indices, bucketptr->buffersize * sizeof(GLuint));
    }

    // queue indices
    i = 0;

    if (plane->indicescount > 0)
    {
        while (i < plane->indicescount)
        {
            bucketptr->indices[bucketptr->count] = plane->indices[i] + plane->mapvbo_vertoffset;
            bucketptr->count++;
            i++;
        }
    }
    else
    {
        static const uint32_t quadindices[6] = { 0, 1, 2, 0, 2, 3 };

        while (i < 6)
        {
            bucketptr->indices[bucketptr->count] = quadindices[i] + plane->mapvbo_vertoffset;
            bucketptr->count++;
            i++;
        }
    }
}

static void         polymer_drawplane(_prplane* plane)
{
    int32_t         materialbits;

    if (pr_nullrender >= 1) return;

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

    if (pr_buckets && pr_vbos > 0 && prcanbucket && plane->bucket)
    {
        polymer_bucketplane(plane);
        return;
    }

    bglNormal3f((float)(plane->plane[0]), (float)(plane->plane[1]), (float)(plane->plane[2]));

    GLuint planevbo;
    GLintptrARB geomfbooffset;

	if (plane->mapvbo_vertoffset != (uint32_t)-1)
    {
        planevbo = prmapvbo;
        geomfbooffset = plane->mapvbo_vertoffset * sizeof(_prvert);
    }
    else
    {
        planevbo = plane->vbo;
        geomfbooffset = 0;
    }

    if (planevbo && (pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, planevbo);
        bglVertexPointer(3, GL_FLOAT, sizeof(_prvert), (GLvoid *)(geomfbooffset));
        bglTexCoordPointer(2, GL_FLOAT, sizeof(_prvert), (GLvoid *)(geomfbooffset + (3 * sizeof(GLfloat))));
        if (plane->indices)
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, plane->ivbo);
    } else {
        bglVertexPointer(3, GL_FLOAT, sizeof(_prvert), &plane->buffer->x);
        bglTexCoordPointer(2, GL_FLOAT, sizeof(_prvert), &plane->buffer->u);
    }

    curlight = 0;
    do {
        materialbits = polymer_bindmaterial(&plane->material, plane->lights, plane->lightcount);

        if (materialbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
        {
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_T, &plane->tbn[0][0]);
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_B, &plane->tbn[1][0]);
            bglVertexAttrib3fvARB(prprograms[materialbits].attrib_N, &plane->tbn[2][0]);
        }

        if (plane->indices)
        {
            if (planevbo && (pr_vbos > 0))
                bglDrawElements(GL_TRIANGLES, plane->indicescount, GL_UNSIGNED_SHORT, NULL);
            else
                bglDrawElements(GL_TRIANGLES, plane->indicescount, GL_UNSIGNED_SHORT, plane->indices);
        } else
            bglDrawArrays(GL_QUADS, 0, 4);

        polymer_unbindmaterial(materialbits);

        if (plane->lightcount && (!depth || mirrors[depth-1].plane))
            prlights[plane->lights[curlight]].flags.isinview = 1;

        curlight++;
    } while ((curlight < plane->lightcount) && (curlight < pr_maxlightpasses) && (!depth || mirrors[depth-1].plane));

    if (planevbo && (pr_vbos > 0))
    {
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        if (plane->indices)
            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
}

static inline void  polymer_inb4mirror(_prvert* buffer, GLfloat* plane)
{
    float           pv;
    float           reflectionmatrix[16];

    pv = buffer->x * plane[0] +
         buffer->y * plane[1] +
         buffer->z * plane[2];

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
            Bfree(prsectors[i]->verts);
            Bfree(prsectors[i]->floor.buffer);
            Bfree(prsectors[i]->ceil.buffer);
            Bfree(prsectors[i]->floor.indices);
            Bfree(prsectors[i]->ceil.indices);
            if (prsectors[i]->ceil.vbo) bglDeleteBuffersARB(1, &prsectors[i]->ceil.vbo);
            if (prsectors[i]->ceil.ivbo) bglDeleteBuffersARB(1, &prsectors[i]->ceil.ivbo);
            if (prsectors[i]->floor.vbo) bglDeleteBuffersARB(1, &prsectors[i]->floor.vbo);
            if (prsectors[i]->floor.ivbo) bglDeleteBuffersARB(1, &prsectors[i]->floor.ivbo);

            DO_FREE_AND_NULL(prsectors[i]);
        }

        i++;
    }

    i = 0;
    while (i < MAXWALLS)
    {
        if (prwalls[i])
        {
            Bfree(prwalls[i]->bigportal);
            Bfree(prwalls[i]->mask.buffer);
            Bfree(prwalls[i]->over.buffer);
            // Bfree(prwalls[i]->cap);
            Bfree(prwalls[i]->wall.buffer);
            if (prwalls[i]->wall.vbo) bglDeleteBuffersARB(1, &prwalls[i]->wall.vbo);
            if (prwalls[i]->over.vbo) bglDeleteBuffersARB(1, &prwalls[i]->over.vbo);
            if (prwalls[i]->mask.vbo) bglDeleteBuffersARB(1, &prwalls[i]->mask.vbo);
            if (prwalls[i]->stuffvbo) bglDeleteBuffersARB(1, &prwalls[i]->stuffvbo);

            DO_FREE_AND_NULL(prwalls[i]);
        }

        i++;
    }

    i = 0;
    while (i < MAXSPRITES)
    {
        if (prsprites[i])
        {
            Bfree(prsprites[i]->plane.buffer);
            if (prsprites[i]->plane.vbo) bglDeleteBuffersARB(1, &prsprites[i]->plane.vbo);
            DO_FREE_AND_NULL(prsprites[i]);
        }

        i++;
    }

    i = 0;
    while (i < MAXTILES)
    {
        polymer_invalidateartmap(i);
        i++;
    }

    i = 0;
    while (i < MAXBASEPALS)
    {
        if (prbasepalmaps[i])
        {
            bglDeleteTextures(1, &prbasepalmaps[i]);
            prbasepalmaps[i] = 0;
        }

        i++;
    }

    i = 0;
    while (i < MAXPALOOKUPS)
    {
        if (prlookups[i])
        {
            bglDeleteTextures(1, &prlookups[i]);
            prlookups[i] = 0;
        }

        i++;
    }
}

// SECTORS
static int32_t      polymer_initsector(int16_t sectnum)
{
    usectortype      *sec;
    _prsector*      s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initializing sector %i...\n", sectnum);

    sec = (usectortype *)&sector[sectnum];
    s = (_prsector *)Xcalloc(1, sizeof(_prsector));

    s->verts = (GLdouble *)Xcalloc(sec->wallnum, sizeof(GLdouble) * 3);
    s->floor.buffer = (_prvert *)Xcalloc(sec->wallnum, sizeof(_prvert));
    s->floor.vertcount = sec->wallnum;
    s->ceil.buffer = (_prvert *)Xcalloc(sec->wallnum, sizeof(_prvert));
    s->ceil.vertcount = sec->wallnum;

    bglGenBuffersARB(1, &s->floor.vbo);
    bglGenBuffersARB(1, &s->ceil.vbo);
    bglGenBuffersARB(1, &s->floor.ivbo);
    bglGenBuffersARB(1, &s->ceil.ivbo);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->floor.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, sec->wallnum * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->ceil.vbo);
    bglBufferDataARB(GL_ARRAY_BUFFER_ARB, sec->wallnum * sizeof(GLfloat) * 5, NULL, mapvbousage);

    bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    s->flags.empty = 1; // let updatesector know that everything needs to go

    prsectors[sectnum] = s;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initialized sector %i.\n", sectnum);

    return 1;
}

static int32_t      polymer_updatesector(int16_t sectnum)
{
    _prsector*      s;
    usectortype      *sec;
    walltype        *wal;
    int32_t         i, j;
    int32_t         ceilz, florz;
    int32_t         tex, tey, heidiff;
    float           secangcos, secangsin, scalecoef, xpancoef, ypancoef;
    int32_t         ang, needfloor, wallinvalidate;
    int16_t         curstat, curpicnum, floorpicnum, ceilingpicnum;
    char            curxpanning, curypanning;
    _prvert*        curbuffer;

    if (pr_nullrender >= 3) return 0;

    s = prsectors[sectnum];
    sec = (usectortype *)&sector[sectnum];

    secangcos = secangsin = 2;

    if (s == NULL)
    {
        if (pr_verbosity >= 1) OSD_Printf("PR : Can't update uninitialized sector %i.\n", sectnum);
        return -1;
    }

    needfloor = wallinvalidate = 0;

    // geometry
    wal = &wall[sec->wallptr];
    i = 0;
    while (i < sec->wallnum)
    {
        if ((-wal->x != s->verts[(i*3)+2]))
        {
            s->verts[(i*3)+2] = s->floor.buffer[i].z = s->ceil.buffer[i].z = -(float)wal->x;
            needfloor = wallinvalidate = 1;
        }
        if ((wal->y != s->verts[i*3]))
        {
            s->verts[i*3] = s->floor.buffer[i].x = s->ceil.buffer[i].x = (float)wal->y;
            needfloor = wallinvalidate = 1;
        }

        i++;
        wal = &wall[sec->wallptr + i];
    }

    if ((s->flags.empty) ||
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
            s->floor.buffer[i].y = -(float)(florz) / 16.0f;
            s->ceil.buffer[i].y = -(float)(ceilz) / 16.0f;

            i++;
            wal = &wall[sec->wallptr + i];
        }

        s->floorz = sec->floorz;
        s->ceilingz = sec->ceilingz;
        s->floorheinum = sec->floorheinum;
        s->ceilingheinum = sec->ceilingheinum;
    }
    else if (sec->visibility != s->visibility)
        wallinvalidate = 1;

    floorpicnum = sec->floorpicnum;
    DO_TILE_ANIM(floorpicnum, sectnum);
    ceilingpicnum = sec->ceilingpicnum;
    DO_TILE_ANIM(ceilingpicnum, sectnum);

    if ((!s->flags.empty) && (!needfloor) &&
            (floorpicnum == s->floorpicnum_anim) &&
            (ceilingpicnum == s->ceilingpicnum_anim) &&
#ifndef UNTRACKED_STRUCTS
            (s->trackedrev == sectorchanged[sectnum]))
#else
            !Bmemcmp(&s->ceilingstat, &sec->ceilingstat, offsetof(sectortype, visibility) - offsetof(sectortype, ceilingstat)))
#endif
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
                xpancoef = (float)(wal->x - wall[sec->wallptr].x);
                ypancoef = (float)(wall[sec->wallptr].y - wal->y);

                tex = (int32_t)(xpancoef * secangsin + ypancoef * secangcos);
                tey = (int32_t)(xpancoef * secangcos - ypancoef * secangsin);
            } else {
                tex = wal->x;
                tey = -wal->y;
            }

            if ((curstat & (2+64)) == (2+64))
            {
                heidiff = (int32_t)(curbuffer[i].y - curbuffer[0].y);
                // don't forget the sign, tey could be negative with concave sectors
                if (tey >= 0)
                    tey = (int32_t)sqrt((double)((tey * tey) + (heidiff * heidiff)));
                else
                    tey = -(int32_t)sqrt((double)((tey * tey) + (heidiff * heidiff)));
            }

            if (curstat & 4)
                swaplong(&tex, &tey);

            if (curstat & 16) tex = -tex;
            if (curstat & 32) tey = -tey;

            scalecoef = (curstat & 8) ? 8.0f : 16.0f;

            if (curxpanning)
            {
                xpancoef = (float)(pow2long[picsiz[curpicnum] & 15]);
                xpancoef *= (float)(curxpanning) / (256.0f * (float)(tilesiz[curpicnum].x));
            }
            else
                xpancoef = 0;

            if (curypanning)
            {
                ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);
                ypancoef *= (float)(curypanning) / (256.0f * (float)(tilesiz[curpicnum].y));
            }
            else
                ypancoef = 0;

            curbuffer[i].u = ((float)(tex) / (scalecoef * tilesiz[curpicnum].x)) + xpancoef;
            curbuffer[i].v = ((float)(tey) / (scalecoef * tilesiz[curpicnum].y)) + ypancoef;

            j--;
        }
        i++;
        wal = &wall[sec->wallptr + i];
    }

    s->floorxpanning = sec->floorxpanning;
    s->ceilingxpanning = sec->ceilingxpanning;
    s->floorypanning = sec->floorypanning;
    s->ceilingypanning = sec->ceilingypanning;
#ifndef UNTRACKED_STRUCTS
    s->trackedrev = sectorchanged[sectnum];
#endif

    i = -1;

attributes:
    if ((pr_vbos > 0) && ((i == -1) || (wallinvalidate)))
    {
        if (pr_vbos > 0)
        {
            if (pr_nullrender < 2)
            {
                /*bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->floor.vbo);
                bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sec->wallnum * sizeof(GLfloat)* 5, s->floor.buffer);
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->ceil.vbo);
                bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, sec->wallnum * sizeof(GLfloat)* 5, s->ceil.buffer);
                */

                s->floor.mapvbo_vertoffset = sec->wallptr * 2;
                s->ceil.mapvbo_vertoffset = s->floor.mapvbo_vertoffset + sec->wallnum;

                GLintptrARB sector_offset = s->floor.mapvbo_vertoffset * sizeof(_prvert);
                GLsizeiptrARB cur_sector_size = sec->wallnum * sizeof(_prvert);
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
                // floor
                bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, sector_offset, cur_sector_size, s->floor.buffer);
                // ceiling
                bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, sector_offset + cur_sector_size, cur_sector_size, s->ceil.buffer);
                bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
            }
        }
        else
        {
            s->floor.mapvbo_vertoffset = -1;
            s->ceil.mapvbo_vertoffset = -1;
        }
    }

    if ((!s->flags.empty) && (!s->flags.invalidtex) &&
            (floorpicnum == s->floorpicnum_anim) &&
            (ceilingpicnum == s->ceilingpicnum_anim) &&
            !Bmemcmp(&s->ceilingstat, &sec->ceilingstat, offsetof(sectortype, visibility) - offsetof(sectortype, ceilingstat)))
        goto finish;

    s->floor.bucket = polymer_getbuildmaterial(&s->floor.material, floorpicnum, sec->floorpal, sec->floorshade, sec->visibility, (sec->floorstat & 384) ? DAMETH_MASK : DAMETH_NOMASK);

    if (sec->floorstat & 256) {
        if (sec->floorstat & 128) {
            s->floor.material.diffusemodulation[3] = 0x55;
        } else {
            s->floor.material.diffusemodulation[3] = 0xAA;
        }
    }

    s->ceil.bucket = polymer_getbuildmaterial(&s->ceil.material, ceilingpicnum, sec->ceilingpal, sec->ceilingshade, sec->visibility, (sec->ceilingstat & 384) ? DAMETH_MASK : DAMETH_NOMASK);

    if (sec->ceilingstat & 256) {
        if (sec->ceilingstat & 128) {
            s->ceil.material.diffusemodulation[3] = 0x55;
        } else {
            s->ceil.material.diffusemodulation[3] = 0xAA;
        }
    }

    s->flags.invalidtex = 0;

    // copy ceilingstat through visibility members
    Bmemcpy(&s->ceilingstat, &sec->ceilingstat, offsetof(sectortype, visibility) - offsetof(sectortype, ceilingstat));
    s->floorpicnum_anim = floorpicnum;
    s->ceilingpicnum_anim = ceilingpicnum;

finish:

    if (needfloor)
    {
        polymer_buildfloor(sectnum);
        if ((pr_vbos > 0))
        {
            if (pr_nullrender < 2)
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
    }

    if (wallinvalidate)
    {
        s->invalidid++;
        polymer_invalidatesectorlights(sectnum);
        polymer_computeplane(&s->floor);
        polymer_computeplane(&s->ceil);
    }

    s->flags.empty = 0;
    s->flags.uptodate = 1;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated sector %i.\n", sectnum);

    return 0;
}

void PR_CALLBACK    polymer_tesserror(GLenum error)
{
    /* This callback is called by the tesselator whenever it raises an error.
       GLU_TESS_ERROR6 is the "no error"/"null" error spam in e1l1 and others. */

    if (pr_verbosity >= 1 && error != GLU_TESS_ERROR6) OSD_Printf("PR : Tesselation error number %i reported : %s.\n", error, bgluErrorString(errno));
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
        s->floor.indices = (GLushort *)Xrealloc(s->floor.indices, s->indicescount * sizeof(GLushort));
        s->ceil.indices = (GLushort *)Xrealloc(s->ceil.indices, s->indicescount * sizeof(GLushort));
    }
    s->ceil.indices[s->curindice] = (intptr_t)vertex;
    s->curindice++;
}

static int32_t      polymer_buildfloor(int16_t sectnum)
{
    // This function tesselates the floor/ceiling of a sector and stores the triangles in a display list.
    _prsector*      s;
    usectortype     *sec;
    intptr_t        i;

    if (pr_verbosity >= 2) OSD_Printf("PR : Tesselating floor of sector %i...\n", sectnum);

    s = prsectors[sectnum];
    sec = (usectortype *)&sector[sectnum];

    if (s == NULL)
        return -1;

    if (s->floor.indices == NULL)
    {
        s->indicescount = (max(3, sec->wallnum) - 2) * 3;
        s->floor.indices = (GLushort *)Xcalloc(s->indicescount, sizeof(GLushort));
        s->ceil.indices = (GLushort *)Xcalloc(s->indicescount, sizeof(GLushort));
    }

    s->curindice = 0;

    bgluTessCallback(prtess, GLU_TESS_VERTEX_DATA, (void (PR_CALLBACK *)(void))polymer_tessvertex);
    bgluTessCallback(prtess, GLU_TESS_EDGE_FLAG, (void (PR_CALLBACK *)(void))polymer_tessedgeflag);
    bgluTessCallback(prtess, GLU_TESS_ERROR, (void (PR_CALLBACK *)(void))polymer_tesserror);

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

    return 1;
}

static void         polymer_drawsector(int16_t sectnum, int32_t domasks)
{
    usectortype     *sec;
    _prsector*      s;
    GLubyte         oldcolor[4];
    int32_t         draw;
    int32_t         queuedmask;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing sector %i...\n", sectnum);

    sec = (usectortype *)&sector[sectnum];
    s = prsectors[sectnum];

    queuedmask = FALSE;

    // If you're thinking of 'optimizing' the following logic, you'd better
    // provide compelling evidence that the generated code is more efficient
    // than what GCC can come up with on its own.

    draw = TRUE;
    // Draw masks regardless; avoid all non-masks TROR links
    if (sec->floorstat & 384) {
        draw = domasks;
    } else if (yax_getbunch(sectnum, YAX_FLOOR) >= 0) {
        draw = FALSE;
    }
    // Parallaxed
    if (sec->floorstat & 1) {
        draw = FALSE;
    }

    if (draw || (searchit == 2)) {
        if (searchit == 2) {
            polymer_drawsearchplane(&s->floor, oldcolor, 0x02, (GLubyte *) &sectnum);
        }
        else {
            calc_and_apply_fog(sec->floorpicnum, fogshade(sec->floorshade, sec->floorpal),
                sec->visibility, get_floor_fogpal(sec));
            polymer_drawplane(&s->floor);
        }
    } else if (!domasks && cursectormaskcount && sec->floorstat & 384) {
        // If we just skipped a mask, queue it for later
        cursectormasks[(*cursectormaskcount)++] = sectnum;
        // Don't queue it twice if the ceiling is also a mask, though.
        queuedmask = TRUE;
    }

    draw = TRUE;
    // Draw masks regardless; avoid all non-masks TROR links
    if (sec->ceilingstat & 384) {
        draw = domasks;
    } else if (yax_getbunch(sectnum, YAX_CEILING) >= 0) {
        draw = FALSE;
    }
    // Parallaxed
    if (sec->ceilingstat & 1) {
        draw = FALSE;
    }

    if (draw || (searchit == 2)) {
        if (searchit == 2) {
            polymer_drawsearchplane(&s->ceil, oldcolor, 0x01, (GLubyte *) &sectnum);
        }
        else {
            calc_and_apply_fog(sec->ceilingpicnum, fogshade(sec->ceilingshade, sec->ceilingpal),
                               sec->visibility, get_ceiling_fogpal(sec));
            polymer_drawplane(&s->ceil);
        }
    } else if (!domasks && !queuedmask && cursectormaskcount &&
               (sec->ceilingstat & 384)) {
        // If we just skipped a mask, queue it for later
        cursectormasks[(*cursectormaskcount)++] = sectnum;
    }

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing sector %i...\n", sectnum);
}

// WALLS
static int32_t      polymer_initwall(int16_t wallnum)
{
    _prwall         *w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initializing wall %i...\n", wallnum);

    w = (_prwall *)Xcalloc(1, sizeof(_prwall));

    if (w->mask.buffer == NULL) {
        w->mask.buffer = (_prvert *)Xmalloc(4 * sizeof(_prvert));
        w->mask.vertcount = 4;
    }
    if (w->bigportal == NULL)
        w->bigportal = (GLfloat *)Xmalloc(4 * sizeof(GLfloat) * 5);
    //if (w->cap == NULL)
    //    w->cap = (GLfloat *)Xmalloc(4 * sizeof(GLfloat) * 3);

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

    w->flags.empty = 1;

    prwalls[wallnum] = w;

    if (pr_verbosity >= 2) OSD_Printf("PR : Initialized wall %i.\n", wallnum);

    return 1;
}

// TODO: r_npotwallmode. Needs polymost_is_npotmode() handling among others.
#define DAMETH_WALL 0

static float calc_ypancoef(char curypanning, int16_t curpicnum, int32_t dopancor)
{
#ifdef NEW_MAP_FORMAT
    if (g_loadedMapVersion >= 10)
        return curypanning / 256.0f;
#endif
    {
        float ypancoef = (float)(pow2long[picsiz[curpicnum] >> 4]);

        if (ypancoef < tilesiz[curpicnum].y)
            ypancoef *= 2;

        if (dopancor)
        {
            int32_t yoffs = Blrintf((ypancoef - tilesiz[curpicnum].y) * (255.0f / ypancoef));
            if (curypanning > 256 - yoffs)
                curypanning -= yoffs;
        }

        ypancoef *= (float)curypanning / (256.0f * (float)tilesiz[curpicnum].y);

        return ypancoef;
    }
}

#define NBYTES_WALL_CSTAT_THROUGH_YPANNING \
    (offsetof(walltype, ypanning)+sizeof(wall[0].ypanning) - offsetof(walltype, cstat))

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
    int32_t         sectofwall = sectorofwall(wallnum);

    if (pr_nullrender >= 3) return;

    // yes, this function is messy and unefficient
    // it also works, bitches
    sec = &sector[sectofwall];

    if (sectofwall < 0 || sectofwall >= numsectors ||
        wallnum < 0 || wallnum > numwalls ||
        sec->wallptr > wallnum || wallnum >= (sec->wallptr + sec->wallnum))
        return; // yay, corrupt map

    wal = &wall[wallnum];
    nwallnum = wal->nextwall;

    w = prwalls[wallnum];
    s = prsectors[sectofwall];
    invalid = s->invalidid;
    if (nwallnum >= 0 && nwallnum < numwalls && wal->nextsector >= 0 && wal->nextsector < numsectors)
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
        w->wall.buffer = (_prvert *)Xcalloc(4, sizeof(_prvert));  // XXX
        w->wall.vertcount = 4;
    }

    wallpicnum = wal->picnum;
    DO_TILE_ANIM(wallpicnum, wallnum+16384);

    walloverpicnum = wal->overpicnum;
    if (walloverpicnum>=0)
        DO_TILE_ANIM(walloverpicnum, wallnum+16384);

    if (nwallnum >= 0 && nwallnum < numwalls)
    {
        nwallpicnum = wall[nwallnum].picnum;
        DO_TILE_ANIM(nwallpicnum, wallnum+16384);
    }
    else
        nwallpicnum = 0;

    if ((!w->flags.empty) && (!w->flags.invalidtex) &&
            (w->invalidid == invalid) &&
            (wallpicnum == w->picnum_anim) &&
            (walloverpicnum == w->overpicnum_anim) &&
#ifndef UNTRACKED_STRUCTS
            (w->trackedrev == wallchanged[wallnum]) &&
#else
            !Bmemcmp(&wal->cstat, &w->cstat, NBYTES_WALL_CSTAT_THROUGH_YPANNING) &&
#endif
            ((nwallnum < 0 || nwallnum > numwalls) ||
             ((nwallpicnum == w->nwallpicnum) &&
              (wall[nwallnum].xpanning == w->nwallxpanning) &&
              (wall[nwallnum].ypanning == w->nwallypanning) &&
              (wall[nwallnum].cstat == w->nwallcstat) &&
              (wall[nwallnum].shade == w->nwallshade))))
    {
        w->flags.uptodate = 1;
        return; // screw you guys I'm going home
    }
    else
    {
        w->invalidid = invalid;

        Bmemcpy(&w->cstat, &wal->cstat, NBYTES_WALL_CSTAT_THROUGH_YPANNING);

        w->picnum_anim = wallpicnum;
        w->overpicnum_anim = walloverpicnum;
#ifndef UNTRACKED_STRUCTS
        w->trackedrev = wallchanged[wallnum];
#endif
        if (nwallnum >= 0 && nwallnum < numwalls)
        {
            w->nwallpicnum = nwallpicnum;
            w->nwallxpanning = wall[nwallnum].xpanning;
            w->nwallypanning = wall[nwallnum].ypanning;
            w->nwallcstat = wall[nwallnum].cstat;
            w->nwallshade = wall[nwallnum].shade;
        }
    }

    w->underover = underwall = overwall = 0;

    if (wal->cstat & 8)
        xref = 1;
    else
        xref = 0;

    if ((unsigned)wal->nextsector >= (unsigned)numsectors || !ns)
    {
        Bmemcpy(w->wall.buffer, &s->floor.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[1], &s->floor.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[2], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
        Bmemcpy(&w->wall.buffer[3], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);

        if (wal->nextsector < 0)
            curpicnum = wallpicnum;
        else
            curpicnum = walloverpicnum;

        w->wall.bucket = polymer_getbuildmaterial(&w->wall.material, curpicnum, wal->pal, wal->shade, sec->visibility, DAMETH_WALL);

        if (wal->cstat & 4)
            yref = sec->floorz;
        else
            yref = sec->ceilingz;

        if ((wal->cstat & 32) && (wal->nextsector >= 0))
        {
            if ((!(wal->cstat & 2) && (wal->cstat & 4)) || ((wal->cstat & 2) && (wall[nwallnum].cstat & 4)))
                yref = sec->ceilingz;
            else
                yref = nsec->floorz;
        }

        if (wal->ypanning)
            // white (but not 1-way)
            ypancoef = calc_ypancoef(wal->ypanning, curpicnum, !(wal->cstat & 4));
        else
            ypancoef = 0;

        i = 0;
        while (i < 4)
        {
            if ((i == 0) || (i == 3))
                dist = (float)xref;
            else
                dist = (float)(xref == 0);

            w->wall.buffer[i].u = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesiz[curpicnum].x);
            w->wall.buffer[i].v = (-(float)(yref + (w->wall.buffer[i].y * 16)) / ((tilesiz[curpicnum].y * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

            if (wal->cstat & 256) w->wall.buffer[i].v = -w->wall.buffer[i].v;

            i++;
        }

        w->underover |= 1;
    }
    else
    {
        nnwallnum = wall[nwallnum].point2;

        if ((s->floor.buffer[wallnum - sec->wallptr].y < ns->floor.buffer[nnwallnum - nsec->wallptr].y) ||
            (s->floor.buffer[wal->point2 - sec->wallptr].y < ns->floor.buffer[nwallnum - nsec->wallptr].y))
            underwall = 1;

        if ((underwall) || (wal->cstat & 16) || (wal->cstat & 32))
        {
            int32_t refwall;

            if (s->floor.buffer[wallnum - sec->wallptr].y < ns->floor.buffer[nnwallnum - nsec->wallptr].y)
                Bmemcpy(w->wall.buffer, &s->floor.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);
            else
                Bmemcpy(w->wall.buffer, &ns->floor.buffer[nnwallnum - nsec->wallptr], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[1], &s->floor.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[2], &ns->floor.buffer[nwallnum - nsec->wallptr], sizeof(GLfloat) * 3);
            Bmemcpy(&w->wall.buffer[3], &ns->floor.buffer[nnwallnum - nsec->wallptr], sizeof(GLfloat) * 3);

            if (wal->cstat & 2)
                refwall = nwallnum;
            else
                refwall = wallnum;

            curpicnum = (wal->cstat & 2) ? nwallpicnum : wallpicnum;
            curpal = wall[refwall].pal;
            curshade = wall[refwall].shade;
            curxpanning = wall[refwall].xpanning;
            curypanning = wall[refwall].ypanning;

            w->wall.bucket = polymer_getbuildmaterial(&w->wall.material, curpicnum, curpal, curshade, sec->visibility, DAMETH_WALL);

            if (!(wall[refwall].cstat&4))
                yref = nsec->floorz;
            else
                yref = sec->ceilingz;

            if (curypanning)
                // under
                ypancoef = calc_ypancoef(curypanning, curpicnum, !(wall[refwall].cstat & 4));
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                if ((i == 0) || (i == 3))
                    dist = (float)xref;
                else
                    dist = (float)(xref == 0);

                w->wall.buffer[i].u = ((dist * 8.0f * wal->xrepeat) + curxpanning) / (float)(tilesiz[curpicnum].x);
                w->wall.buffer[i].v = (-(float)(yref + (w->wall.buffer[i].y * 16)) / ((tilesiz[curpicnum].y * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if ((!(wal->cstat & 2) && (wal->cstat & 256)) ||
                    ((wal->cstat & 2) && (wall[nwallnum].cstat & 256)))
                    w->wall.buffer[i].v = -w->wall.buffer[i].v;

                i++;
            }

            if (underwall)
                w->underover |= 1;

            Bmemcpy(w->mask.buffer, &w->wall.buffer[3], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[1], &w->wall.buffer[2], sizeof(GLfloat) * 5);
        }
        else
        {
            Bmemcpy(w->mask.buffer, &s->floor.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[1], &s->floor.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 5);
        }

        if ((s->ceil.buffer[wallnum - sec->wallptr].y > ns->ceil.buffer[nnwallnum - nsec->wallptr].y) ||
            (s->ceil.buffer[wal->point2 - sec->wallptr].y > ns->ceil.buffer[nwallnum - nsec->wallptr].y))
            overwall = 1;

        if ((overwall) || (wal->cstat & 48))
        {
            if (w->over.buffer == NULL) {
                w->over.buffer = (_prvert *)Xmalloc(4 * sizeof(_prvert));
                w->over.vertcount = 4;
            }

            Bmemcpy(w->over.buffer, &ns->ceil.buffer[nnwallnum - nsec->wallptr], sizeof(GLfloat) * 3);
            Bmemcpy(&w->over.buffer[1], &ns->ceil.buffer[nwallnum - nsec->wallptr], sizeof(GLfloat) * 3);
            if (s->ceil.buffer[wal->point2 - sec->wallptr].y > ns->ceil.buffer[nwallnum - nsec->wallptr].y)
                Bmemcpy(&w->over.buffer[2], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
            else
                Bmemcpy(&w->over.buffer[2], &ns->ceil.buffer[nwallnum - nsec->wallptr], sizeof(GLfloat) * 3);
            Bmemcpy(&w->over.buffer[3], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);

            if ((wal->cstat & 16) || (wal->overpicnum == 0))
                curpicnum = wallpicnum;
            else
                curpicnum = wallpicnum;

            w->over.bucket = polymer_getbuildmaterial(&w->over.material, curpicnum, wal->pal, wal->shade, sec->visibility, DAMETH_WALL);

            if (wal->cstat & 48)
            {
                // mask
                w->mask.bucket = polymer_getbuildmaterial(&w->mask.material, walloverpicnum, wal->pal, wal->shade, sec->visibility, DAMETH_WALL | ((wal->cstat & 48) == 48 ? DAMETH_NOMASK : DAMETH_MASK));

                if (wal->cstat & 128)
                {
                    if (wal->cstat & 512)
                        w->mask.material.diffusemodulation[3] = 0x55;
                    else
                        w->mask.material.diffusemodulation[3] = 0xAA;
                }
            }

            if (wal->cstat & 4)
                yref = sec->ceilingz;
            else
                yref = nsec->ceilingz;

            if (wal->ypanning)
                // over
                ypancoef = calc_ypancoef(wal->ypanning, curpicnum, wal->cstat & 4);
            else
                ypancoef = 0;

            i = 0;
            while (i < 4)
            {
                if ((i == 0) || (i == 3))
                    dist = (float)xref;
                else
                    dist = (float)(xref == 0);

                w->over.buffer[i].u = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesiz[curpicnum].x);
                w->over.buffer[i].v = (-(float)(yref + (w->over.buffer[i].y * 16)) / ((tilesiz[curpicnum].y * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                if (wal->cstat & 256) w->over.buffer[i].v = -w->over.buffer[i].v;

                i++;
            }

            if (overwall)
                w->underover |= 2;

            Bmemcpy(&w->mask.buffer[2], &w->over.buffer[1], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[3], &w->over.buffer[0], sizeof(GLfloat) * 5);

            if ((wal->cstat & 16) || (wal->cstat & 32))
            {
                const int botSwap = (wal->cstat & 4);

                if (wal->cstat & 32)
                {
                    // 1-sided wall
                    if (nsec)
                        yref = botSwap ? sec->ceilingz : nsec->ceilingz;
                    else
                        yref = botSwap ? sec->floorz : sec->ceilingz;
                }
                else
                {
                    // masked wall
                    if (botSwap)
                        yref = min(sec->floorz, nsec->floorz);
                    else
                        yref = max(sec->ceilingz, nsec->ceilingz);
                }

                curpicnum = walloverpicnum;

                if (wal->ypanning)
                    // mask / 1-way
                    ypancoef = calc_ypancoef(wal->ypanning, curpicnum, 0);
                else
                    ypancoef = 0;

                i = 0;
                while (i < 4)
                {
                    if ((i == 0) || (i == 3))
                        dist = (float)xref;
                    else
                        dist = (float)(xref == 0);

                    w->mask.buffer[i].u = ((dist * 8.0f * wal->xrepeat) + wal->xpanning) / (float)(tilesiz[curpicnum].x);
                    w->mask.buffer[i].v = (-(float)(yref + (w->mask.buffer[i].y * 16)) / ((tilesiz[curpicnum].y * 2048.0f) / (float)(wal->yrepeat))) + ypancoef;

                    if (wal->cstat & 256) w->mask.buffer[i].v = -w->mask.buffer[i].v;

                    i++;
                }
            }
        }
        else
        {
            Bmemcpy(&w->mask.buffer[2], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 5);
            Bmemcpy(&w->mask.buffer[3], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 5);
        }
    }

    // make sure shade color handling is correct below XXX
    if (wal->nextsector < 0)
        Bmemcpy(w->mask.buffer, w->wall.buffer, sizeof(_prvert) * 4);

    Bmemcpy(w->bigportal, &s->floor.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[5], &s->floor.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[10], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
    Bmemcpy(&w->bigportal[15], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);

    //Bmemcpy(&w->cap[0], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);
    //Bmemcpy(&w->cap[3], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
    //Bmemcpy(&w->cap[6], &s->ceil.buffer[wal->point2 - sec->wallptr], sizeof(GLfloat) * 3);
    //Bmemcpy(&w->cap[9], &s->ceil.buffer[wallnum - sec->wallptr], sizeof(GLfloat) * 3);
    //w->cap[7] += 1048576; // this number is the result of 1048574 + 2
    //w->cap[10] += 1048576; // this one is arbitrary

    if (w->underover & 1)
        polymer_computeplane(&w->wall);
    if (w->underover & 2)
        polymer_computeplane(&w->over);
    polymer_computeplane(&w->mask);

    if ((pr_vbos > 0))
    {
        if (pr_nullrender < 2)
        {
            const GLintptrARB thiswalloffset = prwalldataoffset + (prwalldatasize * wallnum);
            const GLintptrARB thisoveroffset = thiswalloffset + proneplanesize;
            const GLintptrARB thismaskoffset = thisoveroffset + proneplanesize;
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
            bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, thiswalloffset, proneplanesize, w->wall.buffer);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
            if (w->over.buffer)
                bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, thisoveroffset, proneplanesize, w->over.buffer);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, prmapvbo);
            bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, thismaskoffset, proneplanesize, w->mask.buffer);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
            bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(GLfloat)* 5, w->bigportal);
            //bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat)* 5, 4 * sizeof(GLfloat)* 3, w->cap);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

            w->wall.mapvbo_vertoffset = thiswalloffset / sizeof(_prvert);
            w->over.mapvbo_vertoffset = thisoveroffset / sizeof(_prvert);
            w->mask.mapvbo_vertoffset = thismaskoffset / sizeof(_prvert);
        }
    }
    else
    {
        w->wall.mapvbo_vertoffset = -1;
        w->over.mapvbo_vertoffset = -1;
        w->mask.mapvbo_vertoffset = -1;
    }

    w->flags.empty = 0;
    w->flags.uptodate = 1;
    w->flags.invalidtex = 0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updated wall %i.\n", wallnum);
}

static void         polymer_drawwall(int16_t sectnum, int16_t wallnum)
{
    usectortype     *sec;
    walltype        *wal;
    _prwall         *w;
    GLubyte         oldcolor[4];
    int32_t         parallaxedfloor = 0, parallaxedceiling = 0;

    if (pr_verbosity >= 3) OSD_Printf("PR : Drawing wall %i...\n", wallnum);

    sec = (usectortype *)&sector[sectnum];
    wal = &wall[wallnum];
    w = prwalls[wallnum];

    if ((sec->floorstat & 1) && (wal->nextsector >= 0) &&
        (sector[wal->nextsector].floorstat & 1))
        parallaxedfloor = 1;

    if ((sec->ceilingstat & 1) && (wal->nextsector >= 0) &&
        (sector[wal->nextsector].ceilingstat & 1))
        parallaxedceiling = 1;

    calc_and_apply_fog(wal->picnum, fogshade(wal->shade, wal->pal), sec->visibility, get_floor_fogpal(sec));

    if ((w->underover & 1) && (!parallaxedfloor || (searchit == 2)))
    {
        if (searchit == 2) {
            polymer_drawsearchplane(&w->wall, oldcolor, 0x05, (GLubyte *) &wallnum);
        }
        else
            polymer_drawplane(&w->wall);
    }

    if ((w->underover & 2) && (!parallaxedceiling || (searchit == 2)))
    {
        if (searchit == 2) {
            polymer_drawsearchplane(&w->over, oldcolor, 0x00, (GLubyte *) &wallnum);
        }
        else
            polymer_drawplane(&w->over);
    }

    if ((wall[wallnum].cstat & 32) && (wall[wallnum].nextsector >= 0))
    {
        if (searchit == 2) {
            polymer_drawsearchplane(&w->mask, oldcolor, 0x04, (GLubyte *) &wallnum);
        }
        else
            polymer_drawplane(&w->mask);
    }

    //if (!searchit && (sector[sectnum].ceilingstat & 1) &&
    //    ((wall[wallnum].nextsector < 0) ||
    //    !(sector[wall[wallnum].nextsector].ceilingstat & 1)))
    //{
    //    bglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    //    if (pr_vbos)
    //    {
    //        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, w->stuffvbo);
    //        bglVertexPointer(3, GL_FLOAT, 0, (const GLvoid*)(4 * sizeof(GLfloat) * 5));
    //    }
    //    else
    //        bglVertexPointer(3, GL_FLOAT, 0, w->cap);

    //    bglDrawArrays(GL_QUADS, 0, 4);

    //    if (pr_vbos)
    //        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

    //    bglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    //}

    if (pr_verbosity >= 3) OSD_Printf("PR : Finished drawing wall %i...\n", wallnum);
}

// HSR
static void         polymer_computeplane(_prplane* p)
{
    GLfloat         vec1[5], vec2[5], norm, r;// BxN[3], NxT[3], TxB[3];
    int32_t         i;
    _prvert*        buffer;
    GLfloat*        plane;

    if (p->indices && (p->indicescount < 3))
        return; // corrupt sector (E3L4, I'm looking at you)

    buffer = p->buffer;
    plane = p->plane;

    i = 0;
    do
    {
        vec1[0] = buffer[(INDICE(1))].x - buffer[(INDICE(0))].x; //x1
        vec1[1] = buffer[(INDICE(1))].y - buffer[(INDICE(0))].y; //y1
        vec1[2] = buffer[(INDICE(1))].z - buffer[(INDICE(0))].z; //z1
        vec1[3] = buffer[(INDICE(1))].u - buffer[(INDICE(0))].u; //s1
        vec1[4] = buffer[(INDICE(1))].v - buffer[(INDICE(0))].v; //t1

        vec2[0] = buffer[(INDICE(2))].x - buffer[(INDICE(1))].x; //x2
        vec2[1] = buffer[(INDICE(2))].y - buffer[(INDICE(1))].y; //y2
        vec2[2] = buffer[(INDICE(2))].z - buffer[(INDICE(1))].z; //z2
        vec2[3] = buffer[(INDICE(2))].u - buffer[(INDICE(1))].u; //s2
        vec2[4] = buffer[(INDICE(2))].v - buffer[(INDICE(1))].v; //t2

        polymer_crossproduct(vec2, vec1, plane);

        norm = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];

        // hack to work around a precision issue with slopes
        if (norm >= 15000)
        {
            float tangent[3][3];
            double det;

            // normalize the normal/plane equation and calculate its plane norm
            norm = -sqrt(norm);
            norm = 1.0 / norm;
            plane[0] *= norm;
            plane[1] *= norm;
            plane[2] *= norm;
            plane[3] = -(plane[0] * buffer->x + plane[1] * buffer->y + plane[2] * buffer->z);

            // calculate T and B
            r = 1.0 / (vec1[3] * vec2[4] - vec2[3] * vec1[4]);

            // tangent
            tangent[0][0] = (vec2[4] * vec1[0] - vec1[4] * vec2[0]) * r;
            tangent[0][1] = (vec2[4] * vec1[1] - vec1[4] * vec2[1]) * r;
            tangent[0][2] = (vec2[4] * vec1[2] - vec1[4] * vec2[2]) * r;

            polymer_normalize(&tangent[0][0]);

            // bitangent
            tangent[1][0] = (vec1[3] * vec2[0] - vec2[3] * vec1[0]) * r;
            tangent[1][1] = (vec1[3] * vec2[1] - vec2[3] * vec1[1]) * r;
            tangent[1][2] = (vec1[3] * vec2[2] - vec2[3] * vec1[2]) * r;

            polymer_normalize(&tangent[1][0]);

            // normal
            tangent[2][0] = plane[0];
            tangent[2][1] = plane[1];
            tangent[2][2] = plane[2];

            INVERT_3X3(p->tbn, det, tangent);

            break;
        }
        i+= (p->indices) ? 3 : 1;
    }
    while ((p->indices && i < p->indicescount) ||
          (!p->indices && i < p->vertcount));
}

static inline void  polymer_crossproduct(GLfloat* in_a, GLfloat* in_b, GLfloat* out)
{
    out[0] = in_a[1] * in_b[2] - in_a[2] * in_b[1];
    out[1] = in_a[2] * in_b[0] - in_a[0] * in_b[2];
    out[2] = in_a[0] * in_b[1] - in_a[1] * in_b[0];
}

static inline void  polymer_transformpoint(const float* inpos, float* pos, float* matrix)
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

static inline void  polymer_normalize(float* vec)
{
    double norm;

    norm = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];

    norm = sqrt(norm);
    norm = 1.0 / norm;
    vec[0] *= norm;
    vec[1] *= norm;
    vec[2] *= norm;
}

static inline void  polymer_pokesector(int16_t sectnum)
{
    sectortype      *sec = &sector[sectnum];
    _prsector       *s = prsectors[sectnum];
    walltype        *wal = &wall[sec->wallptr];
    int32_t         i = 0;

    if (!s->flags.uptodate)
        polymer_updatesector(sectnum);

    do
    {
        if ((wal->nextsector >= 0) && (!prsectors[wal->nextsector]->flags.uptodate))
            polymer_updatesector(wal->nextsector);
        if (!prwalls[sec->wallptr + i]->flags.uptodate)
            polymer_updatewall(sec->wallptr + i);

        i++;
        wal = &wall[sec->wallptr + i];
    }
    while (i < sec->wallnum);
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
    do
    {
        uint32_t ii = i<<2, iii = (i<<2) + 3;

        frustum[i] = matrix[iii] + matrix[ii];               // left
        frustum[i + 4] = matrix[iii] - matrix[ii];           // right
        frustum[i + 8] = matrix[iii] - matrix[ii + 1];     // top
        frustum[i + 12] = matrix[iii] + matrix[ii + 1];    // bottom
        frustum[i + 16] = matrix[iii] - matrix[ii + 2];    // far
    }
    while (++i < 4);

    if (pr_verbosity >= 3) OSD_Printf("PR : Frustum extracted.\n");
}

static inline int32_t polymer_planeinfrustum(_prplane *plane, float* frustum)
{
    int32_t         i, j, k = -1;
    i = 4;


    do
    {
        int32_t ii = i * 4;
        j = k = plane->vertcount - 1;

        do
        {
            k -= ((frustum[ii + 0] * plane->buffer[j].x +
                   frustum[ii + 1] * plane->buffer[j].y +
                   frustum[ii + 2] * plane->buffer[j].z +
                   frustum[ii + 3]) < 0.f);
        }
        while (j--);

        if (k == -1)
            return 0; // OUT !
    }
    while (i--);

    return 1;
}

static inline void  polymer_scansprites(int16_t sectnum, uspritetype* localtsprite, int32_t* localspritesortcnt)
{
    int32_t         i;
    spritetype      *spr;

    for (i = headspritesect[sectnum];i >=0;i = nextspritesect[i])
    {
        spr = &sprite[i];
        if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
                (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
                (*localspritesortcnt < maxspritesonscreen))
        {
            // this function's localtsprite is either the tsprite global or
            // polymer_drawroom's locattsprite, so no aliasing
            Bmemcpy(&localtsprite[*localspritesortcnt], spr, sizeof(spritetype));
            localtsprite[*localspritesortcnt].extra = 0;
            localtsprite[(*localspritesortcnt)++].owner = i;
        }
    }
}

void                polymer_updatesprite(int32_t snum)
{
    int32_t         xsize, ysize, i, j;
    int32_t         tilexoff, tileyoff, xoff, yoff, centeryoff=0;
    uspritetype      *tspr = tspriteptr[snum];
    float           xratio, yratio, ang;
    float           spos[3];
    const _prvert   *inbuffer;
    uint8_t         flipu, flipv;
    _prsprite       *s;

    const uint32_t cs = tspr->cstat;
    const uint32_t alignmask = (cs & SPR_ALIGN_MASK);
    const uint8_t flooraligned = (alignmask==SPR_FLOOR);

    if (pr_nullrender >= 3) return;

    if (pr_verbosity >= 3) OSD_Printf("PR : Updating sprite %i...\n", snum);

    int32_t const curpicnum = tspr->picnum;

    if (tspr->owner < 0 || curpicnum < 0) return;

    s = prsprites[tspr->owner];

    if (s == NULL)
    {
        s = prsprites[tspr->owner] = (_prsprite *)Xcalloc(sizeof(_prsprite), 1);

        s->plane.buffer = (_prvert *)Xcalloc(4, sizeof(_prvert));  // XXX
        s->plane.vertcount = 4;
        s->plane.mapvbo_vertoffset = -1;
        s->hash = 0xDEADBEEF;
    }

    if ((tspr->cstat & 48) && (pr_vbos > 0) && !s->plane.vbo)
    {
        if (pr_nullrender < 2)
        {
            bglGenBuffersARB(1, &s->plane.vbo);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->plane.vbo);
            bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(_prvert), NULL, mapvbousage);
        }
    }

    if (tspr->cstat & 48 && searchit != 2)
    {
        uint32_t const changed = XXH32((uint8_t *) tspr, offsetof(spritetype, owner), 0xDEADBEEF);

        if (changed == s->hash)
            return;

        s->hash = changed;
    }

    polymer_getbuildmaterial(&s->plane.material, curpicnum, tspr->pal, tspr->shade,
                             sector[tspr->sectnum].visibility, DAMETH_MASK | DAMETH_CLAMPED);

    if (tspr->cstat & 2)
    {
        if (tspr->cstat & 512)
            s->plane.material.diffusemodulation[3] = 0x55;
        else
            s->plane.material.diffusemodulation[3] = 0xAA;
    }

    float f = s->plane.material.diffusemodulation[3] * (1.0f - spriteext[tspr->owner].alpha);
    s->plane.material.diffusemodulation[3] = (GLubyte)f;

    if (searchit == 2)
    {
        polymer_setupdiffusemodulation(&s->plane, 0x03, (GLubyte *) &tspr->owner);
        s->hash = 0xDEADBEEF;
    }

    if (((tspr->cstat>>4) & 3) == 0)
        xratio = (float)(tspr->xrepeat) * 0.20f; // 32 / 160
    else
        xratio = (float)(tspr->xrepeat) * 0.25f;

    yratio = (float)(tspr->yrepeat) * 0.25f;

    xsize = tilesiz[curpicnum].x;
    ysize = tilesiz[curpicnum].y;

    if (usehightile && h_xsize[curpicnum])
    {
        xsize = h_xsize[curpicnum];
        ysize = h_ysize[curpicnum];
    }

    xsize = (int32_t)(xsize * xratio);
    ysize = (int32_t)(ysize * yratio);

    tilexoff = (int32_t)tspr->xoffset;
    tileyoff = (int32_t)tspr->yoffset;
    tilexoff += (usehightile && h_xsize[curpicnum]) ? h_xoffs[curpicnum] : picanm[curpicnum].xofs;
    tileyoff += (usehightile && h_xsize[curpicnum]) ? h_yoffs[curpicnum] : picanm[curpicnum].yofs;

    xoff = (int32_t)(tilexoff * xratio);
    yoff = (int32_t)(tileyoff * yratio);

    if ((tspr->cstat & 128) && !flooraligned)
    {
        if (alignmask == 0)
            yoff -= ysize / 2;
        else
            centeryoff = ysize / 2;
    }

    spos[0] = (float)tspr->y;
    spos[1] = -(float)(tspr->z) / 16.0f;
    spos[2] = -(float)tspr->x;

    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();

    inbuffer = vertsprite;

    {
        const uint8_t xflip = !!(cs & SPR_XFLIP);
        const uint8_t yflip = !!(cs & SPR_YFLIP);

        // Initially set flipu and flipv.
        flipu = (xflip ^ flooraligned);
        flipv = (yflip && !flooraligned);

        if (pr_billboardingmode && alignmask==0)
        {
            // do surgery on the face tspr to make it look like a wall sprite
            tspr->cstat |= 16;
            tspr->ang = (viewangle + 1024) & 2047;
        }

        if (flipu)
            xoff = -xoff;

        if (yflip && alignmask!=0)
            yoff = -yoff;
    }

    switch (tspr->cstat & SPR_ALIGN_MASK)
    {
    case 0:
        ang = (float)((viewangle) & 2047) * (360.f/2048.f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglRotatef(-horizang, 1.0f, 0.0f, 0.0f);
        bglTranslatef((float)(-xoff), (float)(yoff), 0.0f);
        bglScalef((float)(xsize), (float)(ysize), 1.0f);
        break;
    case SPR_WALL:
        ang = (float)((tspr->ang + 1024) & 2047) * (360.f/2048.f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglTranslatef((float)(-xoff), (float)(yoff-centeryoff), 0.0f);
        bglScalef((float)(xsize), (float)(ysize), 1.0f);
        break;
    case SPR_FLOOR:
        ang = (float)((tspr->ang + 1024) & 2047) * (360.f/2048.f);

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
        bglTranslatef((float)(-xoff), 1.0f, (float)(yoff));
        bglScalef((float)(xsize), 1.0f, (float)(ysize));

        inbuffer = horizsprite;
        break;
    }

    bglGetFloatv(GL_MODELVIEW_MATRIX, spritemodelview);
    bglPopMatrix();

    Bmemcpy(s->plane.buffer, inbuffer, sizeof(_prvert) * 4);

    if (flipu || flipv)
    {
        i = 0;
        do
        {
            if (flipu)
                s->plane.buffer[i].u =
                (s->plane.buffer[i].u - 1.0f) * -1.0f;
            if (flipv)
                s->plane.buffer[i].v =
                (s->plane.buffer[i].v - 1.0f) * -1.0f;
        }
        while (++i < 4);
    }

    i = 0;
    do
        polymer_transformpoint(&inbuffer[i].x, &s->plane.buffer[i].x, spritemodelview);
    while (++i < 4);

    polymer_computeplane(&s->plane);

    if (pr_nullrender < 2)
    {
        if (alignmask && (pr_vbos > 0))
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, s->plane.vbo);
            bglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, 4 * sizeof(_prvert), s->plane.buffer);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
        else if (s->plane.vbo) // clean up the vbo if a wall/floor sprite becomes a face sprite
        {
            bglDeleteBuffersARB(1, &s->plane.vbo);
            s->plane.vbo = 0;
        }
    }

    if (alignmask)
    {
        int32_t curpriority = 0;

        polymer_resetplanelights(&s->plane);

        while (curpriority < pr_maxlightpriority)
        {
            i = j = 0;
            while (j < lightcount)
            {
                while (!prlights[i].flags.active)
                    i++;

                if (prlights[i].priority != curpriority)
                {
                    i++;
                    j++;
                    continue;
                }

                if (polymer_planeinlight(&s->plane, &prlights[i]))
                    polymer_addplanelight(&s->plane, i);
                i++;
                j++;
            }
            curpriority++;
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
            int32_t horizfrac;

            cursky = sector[i].ceilingpicnum;
            curskypal = sector[i].ceilingpal;
            curskyshade = sector[i].ceilingshade;

            getpsky(cursky, &horizfrac, NULL, NULL, NULL);

            switch (horizfrac)
            {
            case 0:
                // psky always at same level wrt screen
                curskyangmul = 0.f;
                break;
            case 65536:
                // psky horiz follows camera horiz
                curskyangmul = 1.f;
                break;
            default:
                // sky has hard-coded parallax
                curskyangmul = 1/DEFAULT_ARTSKY_ANGDIV;
                break;
            }

            return;
        }
        i++;
    }
}

void         polymer_drawsky(int16_t tilenum, char palnum, int8_t shade)
{
    float           pos[3];
    pthtyp*         pth;

    pos[0] = fglobalposy;
    pos[1] = fglobalposz * (-1.f/16.f);
    pos[2] = -fglobalposx;

    bglPushMatrix();
    bglLoadIdentity();

    bglLoadMatrixf(curskymodelviewmatrix);

    bglTranslatef(pos[0], pos[1], pos[2]);
    bglScalef(1000.0f, 1000.0f, 1000.0f);

    drawingskybox = 1;
    pth = texcache_fetch(tilenum, 0, 0, DAMETH_NOMASK);
    drawingskybox = 0;

    if (pth && (pth->flags & PTH_SKYBOX))
        polymer_drawskybox(tilenum, palnum, shade);
    else
        polymer_drawartsky(tilenum, palnum, shade);

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

static void         polymer_drawartsky(int16_t tilenum, char palnum, int8_t shade)
{
    pthtyp*         pth;
    GLuint          glpics[PSKYOFF_MAX+1];
    GLfloat         glcolors[PSKYOFF_MAX+1][3];
    int32_t         i, j;
    GLfloat         height = 2.45f / 2.0f;

    int32_t dapskybits;
    const int8_t *dapskyoff = getpsky(tilenum, NULL, &dapskybits, NULL, NULL);
    const int32_t numskytilesm1 = (1<<dapskybits)-1;

    i = 0;
    while (i <= PSKYOFF_MAX)
    {
        int16_t picnum = tilenum + i;
        // Prevent oob by bad user input:
        if (picnum >= MAXTILES)
            picnum = MAXTILES-1;

        DO_TILE_ANIM(picnum, 0);
        if (!waloff[picnum])
            loadtile(picnum);
        pth = texcache_fetch(picnum, palnum, 0, DAMETH_NOMASK);
        glpics[i] = pth ? pth->glpic : 0;

        glcolors[i][0] = glcolors[i][1] = glcolors[i][2] = getshadefactor(shade);

        if (pth)
        {
            // tinting
            polytintflags_t const tintflags = hictinting[palnum].f;
            if (!(tintflags & HICTINT_PRECOMPUTED))
            {
                if (pth->flags & PTH_HIGHTILE)
                {
                    if (pth->palnum != palnum || (pth->effects & HICTINT_IN_MEMORY) || (tintflags & HICTINT_APPLYOVERALTPAL))
                        hictinting_apply(glcolors[i], palnum);
                }
                else if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
                    hictinting_apply(glcolors[i], palnum);
            }

            // global tinting
            if ((pth->flags & PTH_HIGHTILE) && have_basepal_tint())
                hictinting_apply(glcolors[i], MAXPALOOKUPS-1);
        }

        i++;
    }

    i = 0;
    j = 8;  // In Polymer, an ART sky has always 8 sides...
    while (i < j)
    {
        GLint oldswrap;
        // ... but in case a multi-psky specifies less than 8, repeat cyclically:
        const int8_t tileofs = dapskyoff[i&numskytilesm1];

        bglColor4f(glcolors[tileofs][0], glcolors[tileofs][1], glcolors[tileofs][2], 1.0f);
        bglBindTexture(GL_TEXTURE_2D, glpics[tileofs]);

        bglGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &oldswrap);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);

        polymer_drawartskyquad(i, (i + 1) & (j - 1), height);

        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, oldswrap);

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

static void         polymer_drawskybox(int16_t tilenum, char palnum, int8_t shade)
{
    pthtyp*         pth;
    int32_t         i;
    GLfloat         color[3];

    if ((pr_vbos > 0) && (skyboxdatavbo == 0))
    {
        bglGenBuffersARB(1, &skyboxdatavbo);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, skyboxdatavbo);
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, 4 * sizeof(GLfloat) * 5 * 6, skyboxdata, modelvbousage);

        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }

    if (pr_vbos > 0)
        bglBindBufferARB(GL_ARRAY_BUFFER_ARB, skyboxdatavbo);

    DO_TILE_ANIM(tilenum, 0);

    i = 0;
    while (i < 6)
    {
        drawingskybox = i + 1;
        pth = texcache_fetch(tilenum, palnum, 0, DAMETH_CLAMPED);

        color[0] = color[1] = color[2] = getshadefactor(shade);

        if (pth)
        {
            // tinting
            polytintflags_t const tintflags = hictinting[palnum].f;
            if (!(tintflags & HICTINT_PRECOMPUTED))
            {
                if (pth->flags & PTH_HIGHTILE)
                {
                    if (pth->palnum != palnum || (pth->effects & HICTINT_IN_MEMORY) || (tintflags & HICTINT_APPLYOVERALTPAL))
                        hictinting_apply(color, palnum);
                }
                else if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
                    hictinting_apply(color, palnum);
            }

            // global tinting
            if ((pth->flags & PTH_HIGHTILE) && have_basepal_tint())
                hictinting_apply(color, MAXPALOOKUPS-1);
        }

        bglColor4f(color[0], color[1], color[2], 1.0);
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
static void         polymer_drawmdsprite(uspritetype *tspr)
{
    md3model_t*     m;
    mdskinmap_t*    sk;
    float           *v0, *v1;
    md3surf_t       *s;
    char            targetpal, usinghighpal, foundpalskin;
    float           spos2[3], spos[3], tspos[3], lpos[3], tlpos[3], vec[3], mat[4][4];
    float           ang;
    float           scale;
    double          det;
    int32_t         surfi, i, j;
    GLubyte*        color;
    int32_t         materialbits;
    float           sradius, lradius;
    int16_t         modellights[PR_MAXLIGHTS];
    char            modellightcount;
    uint8_t         curpriority;

    uint8_t lpal = (tspr->owner >= MAXSPRITES) ? tspr->pal : sprite[tspr->owner].pal;

    m = (md3model_t*)models[tile2model[Ptile2tile(tspr->picnum,lpal)].modelid];
    updateanimation((md2model_t *)m,tspr,lpal);

    if ((pr_vbos > 1) && (m->indices == NULL))
        polymer_loadmodelvbos(m);

    // Hackish, but that means it's a model drawn by rotatesprite.
    if (tspriteptr[maxspritesonscreen] == tspr) {
        float       x, y, z;

        spos[0] = fglobalposy;
        spos[1] = fglobalposz * (-1.f/16.f);
        spos[2] = -fglobalposx;

        // The coordinates are actually floats disguised as int in this case
        memcpy(&x, &tspr->x, sizeof(float));
        memcpy(&y, &tspr->y, sizeof(float));
        memcpy(&z, &tspr->z, sizeof(float));

        spos2[0] = y - globalposy;
        spos2[1] = (z - fglobalposz) * (-1.f/16.f);
        spos2[2] = fglobalposx - x;
    } else {
        spos[0] = (float)tspr->y;
        spos[1] = -(float)(tspr->z) / 16.0f;
        spos[2] = -(float)tspr->x;

        spos2[0] = spos2[1] = spos2[2] = 0.0f;
    }

    ang = (float)((tspr->ang+spriteext[tspr->owner].angoff) & 2047) * (360.f/2048.f);
    ang -= 90.0f;
    if (((tspr->cstat>>4) & 3) == 2)
        ang -= 90.0f;

    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();
    scale = (1.0/4.0);
    scale *= m->scale;
    if (pr_overridemodelscale) {
        scale *= pr_overridemodelscale;
    } else {
        scale *= m->bscale;
    }

    if (tspriteptr[maxspritesonscreen] == tspr) {
        float playerang, radplayerang, cosminusradplayerang, sinminusradplayerang, hudzoom;

        playerang = (globalang & 2047) * (360.f/2048.f) - 90.0f;
        radplayerang = (globalang & 2047) * (2.0f * fPI / 2048.0f);
        cosminusradplayerang = cos(-radplayerang);
        sinminusradplayerang = sin(-radplayerang);
        hudzoom = 65536.0 / spriteext[tspr->owner].offset.z;

        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(horizang, -cosminusradplayerang, 0.0f, sinminusradplayerang);
        bglRotatef(spriteext[tspr->owner].roll * (360.f/2048.f), sinminusradplayerang, 0.0f, cosminusradplayerang);
        bglRotatef(-playerang, 0.0f, 1.0f, 0.0f);
        bglScalef(hudzoom, 1.0f, 1.0f);
        bglRotatef(playerang, 0.0f, 1.0f, 0.0f);
        bglTranslatef(spos2[0], spos2[1], spos2[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
    } else {
        bglTranslatef(spos[0], spos[1], spos[2]);
        bglRotatef(-ang, 0.0f, 1.0f, 0.0f);
    }
    if (((tspr->cstat>>4) & 3) == 2)
    {
        bglTranslatef(0.0f, 0.0, -(float)(tilesiz[tspr->picnum].y * tspr->yrepeat) / 8.0f);
        bglRotatef(90.0f, 0.0f, 0.0f, 1.0f);
    }
    else
        bglRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    if ((tspr->cstat & 128) && (((tspr->cstat>>4) & 3) != 2))
        bglTranslatef(0.0f, 0.0, -(float)(tilesiz[tspr->picnum].y * tspr->yrepeat) / 8.0f);

    // yoffset differs from zadd in that it does not follow cstat&8 y-flipping
    bglTranslatef(0.0f, 0.0, m->yoffset * 64 * scale * tspr->yrepeat);

    if (tspr->cstat & 8)
    {
        bglTranslatef(0.0f, 0.0, (float)(tilesiz[tspr->picnum].y * tspr->yrepeat) / 4.0f);
        bglScalef(1.0f, 1.0f, -1.0f);
    }

    if (tspr->cstat & 4)
        bglScalef(1.0f, -1.0f, 1.0f);

    if (!(tspr->cstat & 4) != !(tspr->cstat & 8)) {
        // Only inverting one coordinate will reverse the winding order of
        // faces, so we need to account for that when culling.
        SWITCH_CULL_DIRECTION;
    }

    bglScalef(scale * tspr->xrepeat, scale * tspr->xrepeat, scale * tspr->yrepeat);
    bglTranslatef(0.0f, 0.0, m->zadd * 64);

    // scripted model rotation
    if (tspr->owner < MAXSPRITES &&
        (spriteext[tspr->owner].pitch || spriteext[tspr->owner].roll))
    {
        float       pitchang, rollang, offsets[3];

        pitchang = (float)(spriteext[tspr->owner].pitch) * (360.f/2048.f);
        rollang = (float)(spriteext[tspr->owner].roll) * (360.f/2048.f);

        offsets[0] = -spriteext[tspr->owner].offset.x / (scale * tspr->xrepeat);
        offsets[1] = -spriteext[tspr->owner].offset.y / (scale * tspr->xrepeat);
        offsets[2] = (float)(spriteext[tspr->owner].offset.z) / 16.0f / (scale * tspr->yrepeat);

        bglTranslatef(-offsets[0], -offsets[1], -offsets[2]);

        bglRotatef(pitchang, 0.0f, 1.0f, 0.0f);
        bglRotatef(rollang, -1.0f, 0.0f, 0.0f);

        bglTranslatef(offsets[0], offsets[1], offsets[2]);
    }

    bglGetFloatv(GL_MODELVIEW_MATRIX, spritemodelview);

    bglPopMatrix();
    bglPushMatrix();
    bglMultMatrixf(spritemodelview);

    // invert this matrix to get the polymer -> mdsprite space
    memcpy(mat, spritemodelview, sizeof(float) * 16);
    INVERT_4X4(mdspritespace, det, mat);

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
        (GLubyte)(((float)(numshades-min(max((tspr->shade * shadescale)+m->shadeoff,0),numshades)))/((float)numshades) * 0xFF);

    usinghighpal = (pr_highpalookups &&
                    prhighpalookups[curbasepal][tspr->pal].map);

    // tinting
    polytintflags_t const tintflags = hictinting[tspr->pal].f;
    if (!usinghighpal && !(tintflags & HICTINT_PRECOMPUTED))
    {
        if (!(m->flags&1))
            hictinting_apply_ub(color, tspr->pal);
        else globalnoeffect=1; //mdloadskin reads this
    }

    // global tinting
    if (!usinghighpal && have_basepal_tint())
        hictinting_apply_ub(color, MAXPALOOKUPS-1);

    if (tspr->cstat & 2)
    {
        if (!(tspr->cstat&512))
            color[3] = 0xAA;
        else
            color[3] = 0x55;
    } else
        color[3] = 0xFF;

    {
        double f = color[3] * (1.0f - spriteext[tspr->owner].alpha);
        color[3] = (GLubyte)f;
    }

    if (searchit == 2)
    {
        color[0] = 0x03;
        color[1] = ((GLubyte *)(&tspr->owner))[0];
        color[2] = ((GLubyte *)(&tspr->owner))[1];
        color[3] = 0xFF;
    }

    if (pr_gpusmoothing)
        mdspritematerial.frameprogress = m->interpol;

    mdspritematerial.mdspritespace = GL_TRUE;

    modellightcount = 0;
    curpriority = 0;

    // light culling
    if (lightcount && (!depth || mirrors[depth-1].plane))
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

        while (curpriority < pr_maxlightpriority)
        {
            i = j = 0;
            while (j < lightcount)
            {
                while (!prlights[i].flags.active)
                    i++;

                if (prlights[i].priority != curpriority)
                {
                    i++;
                    j++;
                    continue;
                }

                lradius = prlights[i].range / 1000.0f;

                lpos[0] = (float)prlights[i].y;
                lpos[1] = -(float)prlights[i].z / 16.0f;
                lpos[2] = -(float)prlights[i].x;

                polymer_transformpoint(lpos, tlpos, rootmodelviewmatrix);

                vec[0] = tlpos[0] - spos[0];
                vec[0] *= vec[0];
                vec[1] = tlpos[1] - spos[1];
                vec[1] *= vec[1];
                vec[2] = tlpos[2] - spos[2];
                vec[2] *= vec[2];

                if ((vec[0] + vec[1] + vec[2]) <= ((sradius+lradius) * (sradius+lradius)))
                    modellights[modellightcount++] = i;

                i++;
                j++;
            }
            curpriority++;
        }
    }

    for (surfi=0;surfi<m->head.numsurfs;surfi++)
    {
        s = &m->head.surfs[surfi];
        v0 = &s->geometry[m->cframe*s->numverts*15];
        v1 = &s->geometry[m->nframe*s->numverts*15];

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


        targetpal = tspr->pal;
        foundpalskin = 0;

        for (sk = m->skinmap; sk; sk = sk->next)
            if ((int32_t)sk->palette == tspr->pal &&
                 sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                 sk->surfnum == surfi)
        {
            if (sk->specpower != 1.0)
                mdspritematerial.specmaterial[0] = sk->specpower;
            mdspritematerial.specmaterial[1] = sk->specfactor;
            foundpalskin = 1;
        }

        // If we have a global palette tint, the palskin won't do us any good
        if (curbasepal)
            foundpalskin = 0;

        if (!foundpalskin && usinghighpal) {
            // We don't have a specific skin defined for this palette
            // Use the base skin instead and plug in our highpalookup map
            targetpal = 0;
            mdspritematerial.highpalookupmap = prhighpalookups[curbasepal][tspr->pal].map;
        }

        mdspritematerial.diffusemap =
                mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,targetpal,surfi);
        if (!mdspritematerial.diffusemap)
            continue;

        if (!(tspr->extra&TSPR_EXTRA_MDHACK))
        {
            mdspritematerial.detailmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,DETAILPAL,surfi);

            for (sk = m->skinmap; sk; sk = sk->next)
                if ((int32_t)sk->palette == DETAILPAL &&
                    sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                    sk->surfnum == surfi)
                    mdspritematerial.detailscale[0] = mdspritematerial.detailscale[1] = sk->param;

            mdspritematerial.specmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,SPECULARPAL,surfi);

            mdspritematerial.normalmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,NORMALPAL,surfi);

            for (sk = m->skinmap; sk; sk = sk->next)
                if ((int32_t)sk->palette == NORMALPAL &&
                    sk->skinnum == tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum &&
                    sk->surfnum == surfi) {
                    mdspritematerial.normalbias[0] = sk->specpower;
                    mdspritematerial.normalbias[1] = sk->specfactor;
                }

            mdspritematerial.glowmap =
                    mdloadskin((md2model_t *)m,tile2model[Ptile2tile(tspr->picnum,lpal)].skinnum,GLOWPAL,surfi);
        }

        bglEnableClientState(GL_NORMAL_ARRAY);

        if (pr_vbos > 1)
        {
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->texcoords[surfi]);
            bglTexCoordPointer(2, GL_FLOAT, 0, 0);

            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, m->geometry[surfi]);
            bglVertexPointer(3, GL_FLOAT, sizeof(float) * 15, (GLfloat*)(m->cframe * s->numverts * sizeof(float) * 15));
            bglNormalPointer(GL_FLOAT, sizeof(float) * 15, (GLfloat*)(m->cframe * s->numverts * sizeof(float) * 15) + 3);

            mdspritematerial.tbn = (GLfloat*)(m->cframe * s->numverts * sizeof(float) * 15) + 6;

            if (pr_gpusmoothing) {
                mdspritematerial.nextframedata = (GLfloat*)(m->nframe * s->numverts * sizeof(float) * 15);
            }

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m->indices[surfi]);

            curlight = 0;
            do {
                materialbits = polymer_bindmaterial(&mdspritematerial, modellights, modellightcount);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, 0);
                polymer_unbindmaterial(materialbits);
            } while ((++curlight < modellightcount) && (curlight < pr_maxlightpasses));

            bglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
            bglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        }
        else
        {
            bglVertexPointer(3, GL_FLOAT, sizeof(float) * 15, v0);
            bglNormalPointer(GL_FLOAT, sizeof(float) * 15, v0 + 3);
            bglTexCoordPointer(2, GL_FLOAT, 0, s->uv);

            mdspritematerial.tbn = v0 + 6;

            if (pr_gpusmoothing) {
                mdspritematerial.nextframedata = (GLfloat*)(v1);
            }

            curlight = 0;
            do {
                materialbits = polymer_bindmaterial(&mdspritematerial, modellights, modellightcount);
                bglDrawElements(GL_TRIANGLES, s->numtris * 3, GL_UNSIGNED_INT, s->tris);
                polymer_unbindmaterial(materialbits);
            } while ((++curlight < modellightcount) && (curlight < pr_maxlightpasses));
        }

        bglDisableClientState(GL_NORMAL_ARRAY);
    }

    bglPopMatrix();

    if (!(tspr->cstat & 4) != !(tspr->cstat & 8)) {
        SWITCH_CULL_DIRECTION;
    }

    globalnoeffect = 0;
}

static void         polymer_loadmodelvbos(md3model_t* m)
{
    int32_t         i;
    md3surf_t       *s;

    m->indices = (GLuint *)Xmalloc(m->head.numsurfs * sizeof(GLuint));
    m->texcoords = (GLuint *)Xmalloc(m->head.numsurfs * sizeof(GLuint));
    m->geometry = (GLuint *)Xmalloc(m->head.numsurfs * sizeof(GLuint));

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
        bglBufferDataARB(GL_ARRAY_BUFFER_ARB, s->numframes * s->numverts * sizeof(float) * (15), s->geometry, modelvbousage);

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
    material->nextframedata = (float*)-1;
    // PR_BIT_NORMAL_MAP
    material->normalmap = 0;
    material->normalbias[0] = material->normalbias[1] = 0.0f;
    material->tbn = NULL;
    // PR_BIT_ART_MAP
    material->artmap = 0;
    material->basepalmap = 0;
    material->lookupmap = 0;
    // PR_BIT_DIFFUSE_MAP
    material->diffusemap = 0;
    material->diffusescale[0] = material->diffusescale[1] = 1.0f;
    // PR_BIT_HIGHPALOOKUP_MAP
    material->highpalookupmap = 0;
    // PR_BIT_DIFFUSE_DETAIL_MAP
    material->detailmap = 0;
    material->detailscale[0] = material->detailscale[1] = 1.0f;
    // PR_BIT_DIFFUSE_MODULATION
    material->diffusemodulation[0] =
            material->diffusemodulation[1] =
            material->diffusemodulation[2] =
            material->diffusemodulation[3] = 0xFF;
    // PR_BIT_SPECULAR_MAP
    material->specmap = 0;
    // PR_BIT_SPECULAR_MATERIAL
    material->specmaterial[0] = 15.0f;
    material->specmaterial[1] = 1.0f;
    // PR_BIT_MIRROR_MAP
    material->mirrormap = 0;
    // PR_BIT_GLOW_MAP
    material->glowmap = 0;
    // PR_BIT_PROJECTION_MAP
    material->mdspritespace = GL_FALSE;
}

static void         polymer_setupartmap(int16_t tilenum, char pal)
{
    if (!prartmaps[tilenum]) {
        char *tilebuffer = (char *) waloff[tilenum];
        char *tempbuffer = (char *) Xmalloc(tilesiz[tilenum].x * tilesiz[tilenum].y);
        int i, j, k;

        i = k = 0;
        while (i < tilesiz[tilenum].y) {
            j = 0;
            while (j < tilesiz[tilenum].x) {
                tempbuffer[k] = tilebuffer[(j * tilesiz[tilenum].y) + i];
                k++;
                j++;
            }
            i++;
        }

        bglGenTextures(1, &prartmaps[tilenum]);
        bglBindTexture(GL_TEXTURE_2D, prartmaps[tilenum]);
        bglTexImage2D(GL_TEXTURE_2D,
            0,
            GL_R8,
            tilesiz[tilenum].x,
            tilesiz[tilenum].y,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            tempbuffer);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        bglBindTexture(GL_TEXTURE_2D, 0);
        Bfree(tempbuffer);
    }

    if (!prbasepalmaps[curbasepal]) {
        bglGenTextures(1, &prbasepalmaps[curbasepal]);
        bglBindTexture(GL_TEXTURE_2D, prbasepalmaps[curbasepal]);
        bglTexImage2D(GL_TEXTURE_2D,
            0,
            GL_RGB,
            256,
            1,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            basepaltable[curbasepal]);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glinfo.clamptoedge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
        bglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glinfo.clamptoedge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
        bglBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!prlookups[pal]) {
        bglGenTextures(1, &prlookups[pal]);
        bglBindTexture(GL_TEXTURE_RECTANGLE, prlookups[pal]);
        bglTexImage2D(GL_TEXTURE_RECTANGLE,
            0,
            GL_R8,
            256,
            numshades,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            palookup[pal]);
        bglTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        bglTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, glinfo.clamptoedge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
        bglTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, glinfo.clamptoedge ? GL_CLAMP_TO_EDGE : GL_CLAMP);
        bglBindTexture(GL_TEXTURE_RECTANGLE, 0);
    }
}

static _prbucket*   polymer_getbuildmaterial(_prmaterial* material, int16_t tilenum, char pal, int8_t shade, int8_t vis, int32_t cmeth)
{
    // find corresponding bucket; XXX key that with pr_buckets later, need to be tied to restartvid
    _prbucket *bucketptr = polymer_findbucket(tilenum, pal);

    polymer_getscratchmaterial(material);

    if (!waloff[tilenum])
        loadtile(tilenum);

    // PR_BIT_DIFFUSE_MAP
    pthtyp *pth = texcache_fetch(tilenum, pal, 0, cmeth);

    if (pth)
    {
        material->diffusemap = pth->glpic;

        if (pth->hicr)
        {
            material->diffusescale[0] = pth->hicr->scale.x;
            material->diffusescale[1] = pth->hicr->scale.y;
        }
    }

    int32_t usinghighpal = 0;

    // Lazily fill in all the textures we need, move this to precaching later
    if (pr_artmapping && !(globalflags & GLOBAL_NO_GL_TILESHADES) && polymer_eligible_for_artmap(tilenum, pth))
    {
        polytintflags_t const tintflags = hictinting[pal].f;

        if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
        {
            if (!(tintflags & HICTINT_APPLYOVERPALSWAP))
                pal = 0;
        }

        if (!prartmaps[tilenum] || !prbasepalmaps[curbasepal] || !prlookups[pal])
            polymer_setupartmap(tilenum, pal);

        material->artmap = prartmaps[tilenum];
        material->basepalmap = prbasepalmaps[curbasepal];
        material->lookupmap = prlookups[pal];

        if (!material->basepalmap || !material->lookupmap) {
            material->artmap = 0;
        }

        material->shadeoffset = shade;
        material->visibility = ((uint8_t) (vis+16) / 16.0f);

        // all the stuff below is mutually exclusive with artmapping
        goto done;
    }

    // PR_BIT_HIGHPALOOKUP_MAP
    if (pr_highpalookups && prhighpalookups[curbasepal][pal].map &&
        hicfindsubst(tilenum, 0) &&
        (curbasepal || (hicfindsubst(tilenum, pal)->palnum != pal)))
    {
        material->highpalookupmap = prhighpalookups[curbasepal][pal].map;
        pal = 0;
        usinghighpal = 1;
    }

    if (pth)
    {
        if (pth->hicr)
        {
            // PR_BIT_SPECULAR_MATERIAL
            if (pth->hicr->specpower != 1.0f)
                material->specmaterial[0] = pth->hicr->specpower;
            material->specmaterial[1] = pth->hicr->specfactor;
        }

        // PR_BIT_DIFFUSE_MODULATION
        material->diffusemodulation[0] =
            material->diffusemodulation[1] =
            material->diffusemodulation[2] =
            (GLubyte)(getshadefactor(shade) * 0xFF);

        // tinting
        polytintflags_t const tintflags = hictinting[pal].f;
        if (!(tintflags & HICTINT_PRECOMPUTED))
        {
            if (pth->flags & PTH_HIGHTILE)
            {
                if (pth->palnum != pal || (pth->effects & HICTINT_IN_MEMORY) || (tintflags & HICTINT_APPLYOVERALTPAL))
                    hictinting_apply_ub(material->diffusemodulation, pal);
            }
            else if (tintflags & (HICTINT_USEONART|HICTINT_ALWAYSUSEART))
                hictinting_apply_ub(material->diffusemodulation, pal);
        }

        // global tinting
        if ((pth->flags & PTH_HIGHTILE) && !usinghighpal && have_basepal_tint())
            hictinting_apply_ub(material->diffusemodulation, MAXPALOOKUPS-1);

        // PR_BIT_GLOW_MAP
        if (r_fullbrights && pth->flags & PTH_HASFULLBRIGHT)
            material->glowmap = pth->ofb->glpic;
    }

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (hicfindsubst(tilenum, DETAILPAL, 1) && (pth = texcache_fetch(tilenum, DETAILPAL, 0, DAMETH_NOMASK)) &&
        pth->hicr && (pth->hicr->palnum == DETAILPAL))
    {
        material->detailmap = pth->glpic;
        material->detailscale[0] = pth->hicr->scale.x;
        material->detailscale[1] = pth->hicr->scale.y;
    }

    // PR_BIT_GLOW_MAP
    if (hicfindsubst(tilenum, GLOWPAL, 1) && (pth = texcache_fetch(tilenum, GLOWPAL, 0, DAMETH_MASK)) &&
        pth->hicr && (pth->hicr->palnum == GLOWPAL))
        material->glowmap = pth->glpic;

    // PR_BIT_SPECULAR_MAP
    if (hicfindsubst(tilenum, SPECULARPAL, 1) && (pth = texcache_fetch(tilenum, SPECULARPAL, 0, DAMETH_NOMASK)) &&
        pth->hicr && (pth->hicr->palnum == SPECULARPAL))
        material->specmap = pth->glpic;

    // PR_BIT_NORMAL_MAP
    if (hicfindsubst(tilenum, NORMALPAL, 1) && (pth = texcache_fetch(tilenum, NORMALPAL, 0, DAMETH_NOMASK)) &&
        pth->hicr && (pth->hicr->palnum == NORMALPAL))
    {
        material->normalmap = pth->glpic;
        material->normalbias[0] = pth->hicr->specpower;
        material->normalbias[1] = pth->hicr->specfactor;
    }

done:
    if (bucketptr->invalidmaterial != 0)
    {
        bucketptr->material = *material;
        bucketptr->invalidmaterial = 0;
    }

    return bucketptr;
}

static int32_t      polymer_bindmaterial(const _prmaterial *material, int16_t* lights, int matlightcount)
{
    int32_t         programbits;
    int32_t         texunit;

    programbits = 0;

    // --------- bit validation

    // PR_BIT_ANIM_INTERPOLATION
    if (material->nextframedata != ((float*)-1))
        programbits |= prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit;

    // PR_BIT_LIGHTING_PASS
    if (curlight && matlightcount)
        programbits |= prprogrambits[PR_BIT_LIGHTING_PASS].bit;

    // PR_BIT_NORMAL_MAP
    if (pr_normalmapping && material->normalmap)
        programbits |= prprogrambits[PR_BIT_NORMAL_MAP].bit;

    // PR_BIT_ART_MAP
    if (pr_artmapping && material->artmap &&
        !(globalflags & GLOBAL_NO_GL_TILESHADES) &&
        (overridematerial & prprogrambits[PR_BIT_ART_MAP].bit)) {
        programbits |= prprogrambits[PR_BIT_ART_MAP].bit;
    } else
    // PR_BIT_DIFFUSE_MAP
    if (material->diffusemap) {
        programbits |= prprogrambits[PR_BIT_DIFFUSE_MAP].bit;
        programbits |= prprogrambits[PR_BIT_DIFFUSE_MAP2].bit;
    }

    // PR_BIT_HIGHPALOOKUP_MAP
    if (material->highpalookupmap)
        programbits |= prprogrambits[PR_BIT_HIGHPALOOKUP_MAP].bit;

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (r_detailmapping && material->detailmap)
        programbits |= prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit;

    // PR_BIT_DIFFUSE_MODULATION
    programbits |= prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit;

    // PR_BIT_SPECULAR_MAP
    if (pr_specularmapping && material->specmap)
        programbits |= prprogrambits[PR_BIT_SPECULAR_MAP].bit;

    // PR_BIT_SPECULAR_MATERIAL
    if ((material->specmaterial[0] != 15.0) || (material->specmaterial[1] != 1.0) || pr_overridespecular)
        programbits |= prprogrambits[PR_BIT_SPECULAR_MATERIAL].bit;

    // PR_BIT_MIRROR_MAP
    if (!curlight && material->mirrormap)
        programbits |= prprogrambits[PR_BIT_MIRROR_MAP].bit;

    // PR_BIT_FOG
    if (!material->artmap && !curlight && !material->mirrormap)
        programbits |= prprogrambits[PR_BIT_FOG].bit;

    // PR_BIT_GLOW_MAP
    if (!curlight && r_glowmapping && material->glowmap)
        programbits |= prprogrambits[PR_BIT_GLOW_MAP].bit;

    // PR_BIT_POINT_LIGHT
    if (matlightcount) {
        programbits |= prprogrambits[PR_BIT_POINT_LIGHT].bit;
        // PR_BIT_SPOT_LIGHT
        if (prlights[lights[curlight]].radius) {
            programbits |= prprogrambits[PR_BIT_SPOT_LIGHT].bit;
            // PR_BIT_SHADOW_MAP
            if (prlights[lights[curlight]].rtindex != -1) {
                programbits |= prprogrambits[PR_BIT_SHADOW_MAP].bit;
                programbits |= prprogrambits[PR_BIT_PROJECTION_MAP].bit;
            }
            // PR_BIT_LIGHT_MAP
            if (prlights[lights[curlight]].lightmap) {
                programbits |= prprogrambits[PR_BIT_LIGHT_MAP].bit;
                programbits |= prprogrambits[PR_BIT_PROJECTION_MAP].bit;
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
                                  sizeof(float) * 15,
                                  material->nextframedata);
        if (prprograms[programbits].attrib_nextFrameNormal != -1)
            bglVertexAttribPointerARB(prprograms[programbits].attrib_nextFrameNormal,
                                      3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 15,
                                      material->nextframedata + 3);

        bglUniform1fARB(prprograms[programbits].uniform_frameProgress, material->frameprogress);
    }

    // PR_BIT_LIGHTING_PASS
    if (programbits & prprogrambits[PR_BIT_LIGHTING_PASS].bit)
    {
        bglPushAttrib(GL_COLOR_BUFFER_BIT);
        bglEnable(GL_BLEND);
        bglBlendFunc(GL_ONE, GL_ONE);

        if (prlights[lights[curlight]].publicflags.negative) {
            bglBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        }
    }

    // PR_BIT_NORMAL_MAP
    if (programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
    {
        float pos[3], bias[2];

        pos[0] = fglobalposy;
        pos[1] = fglobalposz * (-1.f/16.f);
        pos[2] = -fglobalposx;

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->normalmap);

        if (material->mdspritespace == GL_TRUE) {
            float mdspritespacepos[3];
            polymer_transformpoint(pos, mdspritespacepos, (float *)mdspritespace);
            bglUniform3fvARB(prprograms[programbits].uniform_eyePosition, 1, mdspritespacepos);
        } else
            bglUniform3fvARB(prprograms[programbits].uniform_eyePosition, 1, pos);
        bglUniform1iARB(prprograms[programbits].uniform_normalMap, texunit);
        if (pr_overrideparallax) {
            bias[0] = pr_parallaxscale;
            bias[1] = pr_parallaxbias;
            bglUniform2fvARB(prprograms[programbits].uniform_normalBias, 1, bias);
        } else
            bglUniform2fvARB(prprograms[programbits].uniform_normalBias, 1, material->normalbias);

        if (material->tbn) {
            bglEnableVertexAttribArrayARB(prprograms[programbits].attrib_T);
            bglEnableVertexAttribArrayARB(prprograms[programbits].attrib_B);
            bglEnableVertexAttribArrayARB(prprograms[programbits].attrib_N);

            bglVertexAttribPointerARB(prprograms[programbits].attrib_T,
                                      3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 15,
                                      material->tbn);
            bglVertexAttribPointerARB(prprograms[programbits].attrib_B,
                                      3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 15,
                                      material->tbn + 3);
            bglVertexAttribPointerARB(prprograms[programbits].attrib_N,
                                      3, GL_FLOAT, GL_FALSE,
                                      sizeof(float) * 15,
                                      material->tbn + 6);
        }

        texunit++;
    }

    // PR_BIT_ART_MAP
    if (programbits & prprogrambits[PR_BIT_ART_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->artmap);

        bglUniform1iARB(prprograms[programbits].uniform_artMap, texunit);

        texunit++;

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->basepalmap);

        bglUniform1iARB(prprograms[programbits].uniform_basePalMap, texunit);

        texunit++;

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_RECTANGLE, material->lookupmap);

        bglUniform1iARB(prprograms[programbits].uniform_lookupMap, texunit);

        texunit++;

        bglUniform1fARB(prprograms[programbits].uniform_shadeOffset, (GLfloat)material->shadeoffset);
        // the furthest visible point is the same as Polymost, however the fog in Polymer is a sphere insted of a plane
        float const visfactor = r_usenewshading == 4 ? 1.f/512.f : 1.f/2048.f;
        bglUniform1fARB(prprograms[programbits].uniform_visibility, globalvisibility * visfactor * material->visibility);
    }

    // PR_BIT_DIFFUSE_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->diffusemap);

        bglUniform1iARB(prprograms[programbits].uniform_diffuseMap, texunit);
        bglUniform2fvARB(prprograms[programbits].uniform_diffuseScale, 1, material->diffusescale);

        texunit++;
    }

    // PR_BIT_HIGHPALOOKUP_MAP
    if (programbits & prprogrambits[PR_BIT_HIGHPALOOKUP_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_3D, material->highpalookupmap);

        bglUniform1iARB(prprograms[programbits].uniform_highPalookupMap, texunit);

        texunit++;
    }

    // PR_BIT_DIFFUSE_DETAIL_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_DETAIL_MAP].bit)
    {
        float scale[2];

        // scale by the diffuse map scale if we're not doing normal mapping
        if (!(programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit))
        {
            scale[0] = material->diffusescale[0] * material->detailscale[0];
            scale[1] = material->diffusescale[1] * material->detailscale[1];
        } else {
            scale[0] = material->detailscale[0];
            scale[1] = material->detailscale[1];
        }

        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->detailmap);

        bglUniform1iARB(prprograms[programbits].uniform_detailMap, texunit);
        bglUniform2fvARB(prprograms[programbits].uniform_detailScale, 1, scale);

        texunit++;
    }

    // PR_BIT_DIFFUSE_MODULATION
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MODULATION].bit)
    {
            bglColor4ub(material->diffusemodulation[0],
                        material->diffusemodulation[1],
                        material->diffusemodulation[2],
                        material->diffusemodulation[3]);
    }

    // PR_BIT_SPECULAR_MAP
    if (programbits & prprogrambits[PR_BIT_SPECULAR_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->specmap);

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
            bglUniform2fvARB(prprograms[programbits].uniform_specMaterial, 1, material->specmaterial);
    }

    // PR_BIT_MIRROR_MAP
    if (programbits & prprogrambits[PR_BIT_MIRROR_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_RECTANGLE, material->mirrormap);

        bglUniform1iARB(prprograms[programbits].uniform_mirrorMap, texunit);

        texunit++;
    }
#ifdef PR_LINEAR_FOG
    if (programbits & prprogrambits[PR_BIT_FOG].bit)
    {
        bglUniform1iARB(prprograms[programbits].uniform_linearFog, r_usenewshading >= 2);
    }
#endif
    // PR_BIT_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_GLOW_MAP].bit)
    {
        bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
        bglBindTexture(GL_TEXTURE_2D, material->glowmap);

        bglUniform1iARB(prprograms[programbits].uniform_glowMap, texunit);

        texunit++;
    }

    // PR_BIT_POINT_LIGHT
    if (programbits & prprogrambits[PR_BIT_POINT_LIGHT].bit)
    {
        float inpos[4], pos[4];
        float range[2];
        float color[4];

        inpos[0] = (float)prlights[lights[curlight]].y;
        inpos[1] = -(float)prlights[lights[curlight]].z / 16.0f;
        inpos[2] = -(float)prlights[lights[curlight]].x;

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

            // PR_BIT_PROJECTION_MAP
            if (programbits & prprogrambits[PR_BIT_PROJECTION_MAP].bit)
            {
                GLfloat matrix[16];

                bglMatrixMode(GL_TEXTURE);
                bglLoadMatrixf(shadowBias);
                bglMultMatrixf(prlights[lights[curlight]].proj);
                bglMultMatrixf(prlights[lights[curlight]].transform);
                if (material->mdspritespace == GL_TRUE)
                    bglMultMatrixf(spritemodelview);
                bglGetFloatv(GL_TEXTURE_MATRIX, matrix);
                bglLoadIdentity();
                bglMatrixMode(GL_MODELVIEW);

                bglUniformMatrix4fvARB(prprograms[programbits].uniform_shadowProjMatrix, 1, GL_FALSE, matrix);

                // PR_BIT_SHADOW_MAP
                if (programbits & prprogrambits[PR_BIT_SHADOW_MAP].bit)
                {
                    bglActiveTextureARB(texunit + GL_TEXTURE0_ARB);
                    bglBindTexture(prrts[prlights[lights[curlight]].rtindex].target, prrts[prlights[lights[curlight]].rtindex].z);

                    bglUniform1iARB(prprograms[programbits].uniform_shadowMap, texunit);

                    texunit++;
                }

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
        range[1] = 1 / (range[0] * range[0]);

        color[0] = prlights[lights[curlight]].color[0]   / 255.0f;
        color[1] = prlights[lights[curlight]].color[1]   / 255.0f;
        color[2] = prlights[lights[curlight]].color[2]   / 255.0f;

        // If this isn't a lighting-only pass, just negate the components
        if (!curlight && prlights[lights[curlight]].publicflags.negative) {
            color[0] = -color[0];
            color[1] = -color[1];
            color[2] = -color[2];
        }

        bglLightfv(GL_LIGHT0, GL_AMBIENT, pos);
        bglLightfv(GL_LIGHT0, GL_DIFFUSE, color);
        if (material->mdspritespace == GL_TRUE) {
            float mdspritespacepos[3];
            polymer_transformpoint(inpos, mdspritespacepos, (float *)mdspritespace);
            bglLightfv(GL_LIGHT0, GL_SPECULAR, mdspritespacepos);
        } else {
            bglLightfv(GL_LIGHT0, GL_SPECULAR, inpos);
        }
        bglLightfv(GL_LIGHT0, GL_LINEAR_ATTENUATION, &range[1]);
    }

    bglActiveTextureARB(GL_TEXTURE0_ARB);

    return programbits;
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
        bglPopAttrib();
    }

    // PR_BIT_NORMAL_MAP
    if (programbits & prprogrambits[PR_BIT_NORMAL_MAP].bit)
    {
        bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_T);
        bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_B);
        bglDisableVertexAttribArrayARB(prprograms[programbits].attrib_N);
    }

    bglUseProgramObjectARB(0);
}

static void         polymer_compileprogram(int32_t programbits)
{
    int32_t         i, enabledbits;
    GLhandleARB     vert, frag, program;
    const GLcharARB*      source[PR_BIT_COUNT * 2];
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

    bglShaderSourceARB(vert, enabledbits, source, NULL);

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

#ifdef DEBUGGINGAIDS
    if (pr_verbosity >= 1)
#else
    if (pr_verbosity >= 2)
#endif
        OSD_Printf("PR : Compiling GPU program with bits (octal) %o...\n", (unsigned)programbits);
    if (!linkstatus) {
        OSD_Printf("PR : Failed to compile GPU program with bits (octal) %o!\n", (unsigned)programbits);
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

    // PR_BIT_ART_MAP
    if (programbits & prprogrambits[PR_BIT_ART_MAP].bit)
    {
        prprograms[programbits].uniform_artMap = bglGetUniformLocationARB(program, "artMap");
        prprograms[programbits].uniform_basePalMap = bglGetUniformLocationARB(program, "basePalMap");
        prprograms[programbits].uniform_lookupMap = bglGetUniformLocationARB(program, "lookupMap");
        prprograms[programbits].uniform_shadeOffset = bglGetUniformLocationARB(program, "shadeOffset");
        prprograms[programbits].uniform_visibility = bglGetUniformLocationARB(program, "visibility");
    }

    // PR_BIT_DIFFUSE_MAP
    if (programbits & prprogrambits[PR_BIT_DIFFUSE_MAP].bit)
    {
        prprograms[programbits].uniform_diffuseMap = bglGetUniformLocationARB(program, "diffuseMap");
        prprograms[programbits].uniform_diffuseScale = bglGetUniformLocationARB(program, "diffuseScale");
    }

    // PR_BIT_HIGHPALOOKUP_MAP
    if (programbits & prprogrambits[PR_BIT_HIGHPALOOKUP_MAP].bit)
    {
        prprograms[programbits].uniform_highPalookupMap = bglGetUniformLocationARB(program, "highPalookupMap");
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
#ifdef PR_LINEAR_FOG
    if (programbits & prprogrambits[PR_BIT_FOG].bit)
    {
        prprograms[programbits].uniform_linearFog = bglGetUniformLocationARB(program, "linearFog");
    }
#endif
    // PR_BIT_GLOW_MAP
    if (programbits & prprogrambits[PR_BIT_GLOW_MAP].bit)
    {
        prprograms[programbits].uniform_glowMap = bglGetUniformLocationARB(program, "glowMap");
    }

    // PR_BIT_PROJECTION_MAP
    if (programbits & prprogrambits[PR_BIT_PROJECTION_MAP].bit)
    {
        prprograms[programbits].uniform_shadowProjMatrix = bglGetUniformLocationARB(program, "shadowProjMatrix");
    }

    // PR_BIT_SHADOW_MAP
    if (programbits & prprogrambits[PR_BIT_SHADOW_MAP].bit)
    {
        prprograms[programbits].uniform_shadowMap = bglGetUniformLocationARB(program, "shadowMap");
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
static void         polymer_removelight(int16_t lighti)
{
    _prplanelist*   oldhead;

    while (prlights[lighti].planelist)
    {
        polymer_deleteplanelight(prlights[lighti].planelist->plane, lighti);
        oldhead = prlights[lighti].planelist;
        prlights[lighti].planelist = prlights[lighti].planelist->n;
        oldhead->n = plpool;
        plpool = oldhead;
        plpool->plane = NULL;
    }
    prlights[lighti].planecount = 0;
    prlights[lighti].planelist = NULL;
}

static void         polymer_updatelights(void)
{
    int32_t         i = 0;

    do
    {
        _prlight* light = &prlights[i];

        if (light->flags.active && light->flags.invalidate) {
            // highly suboptimal
            polymer_removelight(i);

            if (light->radius)
                polymer_processspotlight(light);

            polymer_culllight(i);

            light->flags.invalidate = 0;
        }

        if (light->flags.active) {
            // get the texture handle for the lightmap
            if (light->radius && light->tilenum > 0)
            {
                int16_t     picnum = light->tilenum;
                pthtyp*     pth;

                DO_TILE_ANIM(picnum, 0);

                if (!waloff[picnum])
                    loadtile(picnum);

                pth = NULL;
                pth = texcache_fetch(picnum, 0, 0, DAMETH_NOMASK);

                if (pth)
                    light->lightmap = pth->glpic;
            }

            light->rtindex = -1;
        }
    }
    while (++i < PR_MAXLIGHTS);
}

static inline void  polymer_resetplanelights(_prplane* plane)
{
    Bmemset(&plane->lights[0], -1, sizeof(plane->lights[0]) * plane->lightcount);
    plane->lightcount = 0;
}

static void         polymer_addplanelight(_prplane* plane, int16_t lighti)
{
    _prplanelist*   oldhead;
    int32_t         i = 0;

    if (plane->lightcount)
    {
        if (plane->lightcount == PR_MAXLIGHTS - 1)
            return;

        do
        {
            if (plane->lights[i++] == lighti)
                goto out;
        }
        while (i < plane->lightcount);

        i = 0;
        while (i < plane->lightcount && prlights[plane->lights[i]].priority < prlights[lighti].priority)
            i++;
        Bmemmove(&plane->lights[i+1], &plane->lights[i], sizeof(int16_t) * (plane->lightcount - i));
    }

    plane->lights[i] = lighti;
    plane->lightcount++;

out:
    oldhead = prlights[lighti].planelist;
    while (oldhead != NULL)
    {
        if (oldhead->plane == plane) return;
        oldhead = oldhead->n;
    }

    oldhead = prlights[lighti].planelist;
    if (plpool == NULL)
    {
        prlights[lighti].planelist = (_prplanelist *) Xmalloc(sizeof(_prplanelist));
        prlights[lighti].planelist->n = oldhead;
    }
    else
    {
        prlights[lighti].planelist = plpool;
        plpool = plpool->n;
        prlights[lighti].planelist->n = oldhead;
    }

    prlights[lighti].planelist->plane = plane;
    prlights[lighti].planecount++;
}

static inline void  polymer_deleteplanelight(_prplane* plane, int16_t lighti)
{
    int32_t         i = plane->lightcount-1;

    while (i >= 0)
    {
        if (plane->lights[i] == lighti)
        {
            Bmemmove(&plane->lights[i], &plane->lights[i+1], sizeof(int16_t) * (plane->lightcount - i));
            plane->lightcount--;
            return;
        }
        i--;
    }
}

static int32_t      polymer_planeinlight(_prplane* plane, _prlight* light)
{
    float           lightpos[3];
    int32_t         i, j, k, l;

    if (!plane->vertcount)
        return 0;

    if (light->radius)
        return polymer_planeinfrustum(plane, light->frustum);

    lightpos[0] = (float)light->y;
    lightpos[1] = -(float)light->z / 16.0f;
    lightpos[2] = -(float)light->x;

    i = 0;

    do
    {
        j = k = l = 0;

        do
        {
            if ((&plane->buffer[j].x)[i] > (lightpos[i] + light->range)) k++;
            if ((&plane->buffer[j].x)[i] < (lightpos[i] - light->range)) l++;
        }
        while (++j < plane->vertcount);

        if ((k == plane->vertcount) || (l == plane->vertcount))
            return 0;
    }
    while (++i < 3);

    return 1;
}

static void         polymer_invalidateplanelights(_prplane* plane)
{
    int32_t         i = plane->lightcount;

    while (i--)
    {
        if (((unsigned)plane->lights[i] < PR_MAXLIGHTS) && (prlights[plane->lights[i]].flags.active))
            prlights[plane->lights[i]].flags.invalidate = 1;
    }
}

static void         polymer_invalidatesectorlights(int16_t sectnum)
{
    int32_t         i;
    _prsector       *s = prsectors[sectnum];
    sectortype      *sec = &sector[sectnum];

    if (!s)
        return;

    polymer_invalidateplanelights(&s->floor);
    polymer_invalidateplanelights(&s->ceil);

    i = sec->wallnum;

    while (i--)
    {
        _prwall         *w;
        if (!(w = prwalls[sec->wallptr + i])) continue;

        polymer_invalidateplanelights(&w->wall);
        polymer_invalidateplanelights(&w->over);
        polymer_invalidateplanelights(&w->mask);
    }
}

static void         polymer_processspotlight(_prlight* light)
{
    float           radius, ang, horizang, lightpos[3];

    // hack to avoid lights beams perpendicular to walls
    if ((light->horiz <= 100) && (light->horiz > 90))
        light->horiz = 90;
    if ((light->horiz > 100) && (light->horiz < 110))
        light->horiz = 110;

    lightpos[0] = (float)light->y;
    lightpos[1] = -(float)light->z / 16.0f;
    lightpos[2] = -(float)light->x;

    // calculate the spot light transformations and matrices
    radius = (float)(light->radius) * (360.f/2048.f);
    ang = (float)(light->angle) * (360.f/2048.f);
    horizang = (float)(-getangle(128, light->horiz-100)) * (360.f/2048.f);

    bglMatrixMode(GL_PROJECTION);
    bglPushMatrix();
    bglLoadIdentity();
    bgluPerspective(radius * 2, 1, 0.1f, light->range * (1.f/1000.f));
    bglGetFloatv(GL_PROJECTION_MATRIX, light->proj);
    bglPopMatrix();

    bglMatrixMode(GL_MODELVIEW);
    bglPushMatrix();
    bglLoadIdentity();
    bglRotatef(horizang, 1.0f, 0.0f, 0.0f);
    bglRotatef(ang, 0.0f, 1.0f, 0.0f);
    bglScalef(1.0f / 1000.0f, 1.0f / 1000.0f, 1.0f / 1000.0f);
    bglTranslatef(-lightpos[0], -lightpos[1], -lightpos[2]);
    bglGetFloatv(GL_MODELVIEW_MATRIX, light->transform);
    bglPopMatrix();

    polymer_extractfrustum(light->transform, light->proj, light->frustum);

    light->rtindex = -1;
    light->lightmap = 0;
}

static inline void  polymer_culllight(int16_t lighti)
{
    _prlight*       light = &prlights[lighti];
    int32_t         front = 0;
    int32_t         back = 1;
    int32_t         i;
    int32_t         j;
    int32_t         zdiff;
    int32_t         checkror;
    int16_t         bunchnum;
    int16_t         ns;
    _prsector       *s;
    _prwall         *w;
    sectortype      *sec;

    Bmemset(drawingstate, 0, sizeof(int16_t) * numsectors);
    drawingstate[light->sector] = 1;

    sectorqueue[0] = light->sector;

    do
    {
        s = prsectors[sectorqueue[front]];
        sec = &sector[sectorqueue[front]];

        polymer_pokesector(sectorqueue[front]);

        checkror = FALSE;

        zdiff = light->z - s->floorz;
        if (zdiff < 0)
            zdiff = -zdiff;
        zdiff >>= 4;

        if (!light->radius && !(sec->floorstat & 1)) {
            if (zdiff < light->range) {
                polymer_addplanelight(&s->floor, lighti);
                checkror = TRUE;
            }
        } else if (polymer_planeinlight(&s->floor, light)) {
            polymer_addplanelight(&s->floor, lighti);
            checkror = TRUE;
        }

#ifdef YAX_ENABLE
        // queue ROR neighbors
        if (checkror &&
            (bunchnum = yax_getbunch(sectorqueue[front], YAX_FLOOR)) >= 0) {

            for (SECTORS_OF_BUNCH(bunchnum, YAX_CEILING, ns)) {

                if (ns >= 0 && !drawingstate[ns] &&
                    polymer_planeinlight(&prsectors[ns]->ceil, light)) {

                    sectorqueue[back++] = ns;
                    drawingstate[ns] = 1;
                }
            }
        }
#endif
        checkror = FALSE;

        zdiff = light->z - s->ceilingz;
        if (zdiff < 0)
            zdiff = -zdiff;
        zdiff >>= 4;

        if (!light->radius && !(sec->ceilingstat & 1)) {
            if (zdiff < light->range) {
                polymer_addplanelight(&s->ceil, lighti);
                checkror = TRUE;
            }
        } else if (polymer_planeinlight(&s->ceil, light)) {
            polymer_addplanelight(&s->ceil, lighti);
            checkror = TRUE;
        }

#ifdef YAX_ENABLE
        // queue ROR neighbors
        if (checkror &&
            (bunchnum = yax_getbunch(sectorqueue[front], YAX_CEILING)) >= 0) {

            for (SECTORS_OF_BUNCH(bunchnum, YAX_FLOOR, ns)) {

                if (ns >= 0 && !drawingstate[ns] &&
                    polymer_planeinlight(&prsectors[ns]->floor, light)) {

                    sectorqueue[back++] = ns;
                    drawingstate[ns] = 1;
                }
            }
        }
#endif
        i = 0;
        while (i < sec->wallnum)
        {
            w = prwalls[sec->wallptr + i];

            j = 0;

            if (polymer_planeinlight(&w->wall, light)) {
                polymer_addplanelight(&w->wall, lighti);
                j++;
            }

            if (polymer_planeinlight(&w->over, light)) {
                polymer_addplanelight(&w->over, lighti);
                j++;
            }

            // assume the light hits the middle section if it hits the top and bottom
            if (wallvisible(light->x, light->y, sec->wallptr + i) &&
                (j == 2 || polymer_planeinlight(&w->mask, light))) {
                if ((w->mask.vertcount == 4) &&
                    (w->mask.buffer[0].y >= w->mask.buffer[3].y) &&
                    (w->mask.buffer[1].y >= w->mask.buffer[2].y))
                {
                    i++;
                    continue;
                }

                polymer_addplanelight(&w->mask, lighti);

                if ((wall[sec->wallptr + i].nextsector >= 0) &&
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
    while (front != back);

    i = MAXSPRITES-1;

    do
    {
        _prsprite *s = prsprites[i];

        if ((sprite[i].cstat & 48) == 0 || s == NULL || sprite[i].statnum == MAXSTATUS || sprite[i].sectnum == MAXSECTORS)
            continue;

        if (polymer_planeinlight(&s->plane, light))
            polymer_addplanelight(&s->plane, lighti);
    }
    while (i--);
}

static void         polymer_prepareshadows(void)
{
    int16_t         oviewangle, oglobalang;
    int32_t         i, j, k;
    int32_t         gx, gy, gz;
    int32_t         oldoverridematerial;

    // for wallvisible()
    gx = globalposx;
    gy = globalposy;
    gz = globalposz;
    // build globals used by drawmasks
    oviewangle = viewangle;
    oglobalang = globalang;

    i = j = k = 0;

    while ((k < lightcount) && (j < pr_shadowcount))
    {
        while (!prlights[i].flags.active)
            i++;

        if (prlights[i].radius && prlights[i].publicflags.emitshadow &&
            prlights[i].flags.isinview)
        {
            prlights[i].flags.isinview = 0;
            prlights[i].rtindex = j + 1;
            if (pr_verbosity >= 3) OSD_Printf("PR : Drawing shadow %i...\n", i);

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

            set_globalpos(prlights[i].x, prlights[i].y, prlights[i].z);

            // build globals used by rotatesprite
            viewangle = prlights[i].angle;
            set_globalang(prlights[i].angle);

            oldoverridematerial = overridematerial;
            // smooth model shadows
            overridematerial = prprogrambits[PR_BIT_ANIM_INTERPOLATION].bit;
            // used by alpha-testing for sprite silhouette
            overridematerial |= prprogrambits[PR_BIT_DIFFUSE_MAP].bit;
            overridematerial |= prprogrambits[PR_BIT_DIFFUSE_MAP2].bit;

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
        k++;
    }

    set_globalpos(gx, gy, gz);

    viewangle = oviewangle;
    set_globalang(oglobalang);
}

// RENDER TARGETS
static void         polymer_initrendertargets(int32_t count)
{
    int32_t         i;

    static int32_t ocount;

    if (count == 0)  // uninit
    {
        if (prrts)
        {
            for (i=0; i<ocount; i++)
            {
                if (prrts[i].color)
                {
                    bglDeleteTextures(1, &prrts[i].color);
                    prrts[i].color = 0;
                }
                bglDeleteTextures(1, &prrts[i].z);
                prrts[i].z = 0;

                bglDeleteFramebuffersEXT(1, &prrts[i].fbo);
                prrts[i].fbo = 0;
            }
            DO_FREE_AND_NULL(prrts);
        }

        ocount = 0;
        return;
    }

    ocount = count;
    //////////

    prrts = (_prrt *)Xcalloc(count, sizeof(_prrt));

    i = 0;
    while (i < count)
    {
        if (!i) {
            prrts[i].target = GL_TEXTURE_RECTANGLE;
            prrts[i].xdim = xdim;
            prrts[i].ydim = ydim;

            bglGenTextures(1, &prrts[i].color);
            bglBindTexture(prrts[i].target, prrts[i].color);

            bglTexImage2D(prrts[i].target, 0, GL_RGB, prrts[i].xdim, prrts[i].ydim, 0, GL_RGB, GL_SHORT, NULL);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
            bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        } else {
            prrts[i].target = GL_TEXTURE_2D;
            prrts[i].xdim = 128 << pr_shadowdetail;
            prrts[i].ydim = 128 << pr_shadowdetail;
            prrts[i].color = 0;

            if (pr_ati_fboworkaround) {
                bglGenTextures(1, &prrts[i].color);
                bglBindTexture(prrts[i].target, prrts[i].color);

                bglTexImage2D(prrts[i].target, 0, GL_RGB, prrts[i].xdim, prrts[i].ydim, 0, GL_RGB, GL_SHORT, NULL);
                bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
                bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
            }
        }

        bglGenTextures(1, &prrts[i].z);
        bglBindTexture(prrts[i].target, prrts[i].z);

        bglTexImage2D(prrts[i].target, 0, GL_DEPTH_COMPONENT, prrts[i].xdim, prrts[i].ydim, 0, GL_DEPTH_COMPONENT, GL_SHORT, NULL);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_MIN_FILTER, pr_shadowfiltering ? GL_LINEAR : GL_NEAREST);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_MAG_FILTER, pr_shadowfiltering ? GL_LINEAR : GL_NEAREST);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_S, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
        bglTexParameteri(prrts[i].target, GL_TEXTURE_WRAP_T, glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
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

// DEBUG OUTPUT
void PR_CALLBACK    polymer_debugoutputcallback(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam)
{
    UNREFERENCED_PARAMETER(source);
    UNREFERENCED_PARAMETER(type);
    UNREFERENCED_PARAMETER(id);
    UNREFERENCED_PARAMETER(severity);
    UNREFERENCED_PARAMETER(length);
    UNREFERENCED_PARAMETER(userParam);

    if (type == GL_DEBUG_TYPE_ERROR_ARB)
    {
        OSD_Printf("PR : Received OpenGL debug message: %s\n", message);
    }
}

#endif
