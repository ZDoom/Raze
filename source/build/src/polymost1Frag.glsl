#version 110
#extension ARB_shader_texture_lod : enable

//include an additional space here so that we can programmatically search for and disable this preprocessor definition easily
 #define POLYMOST1_EXTENDED

//s_texture points to an indexed color texture
uniform sampler2D s_texture;
//s_palswap is the palette swap texture where u is the color index and v is the shade
uniform sampler2D s_palswap;
//s_palette is the base palette texture where u is the color index
uniform sampler2D s_palette;

#ifdef POLYMOST1_EXTENDED
uniform sampler2D s_detail;
uniform sampler2D s_glow;
#endif

//u_texturePosSize is the texture position & size packaged into a single vec4 as {pos.x, pos.y, size.x, size.y}
uniform vec4 u_texturePosSize;
uniform vec2 u_halfTexelSize;
uniform vec2 u_palswapPos;
uniform vec2 u_palswapSize;

uniform float u_shade;
uniform float u_numShades;
uniform float u_visFactor;
uniform float u_fogEnabled;

uniform float u_useColorOnly;
uniform float u_usePalette;
uniform float u_npotEmulation;
uniform float u_npotEmulationFactor;
uniform float u_npotEmulationXOffset;
uniform float u_shadeInterpolate;

#ifdef POLYMOST1_EXTENDED
uniform float u_useDetailMapping;
uniform float u_useGlowMapping;
#endif

varying vec4 v_color;
varying float v_distance;

const float c_basepalScale = 255.0/256.0;
const float c_basepalOffset = 0.5/256.0;

const float c_zero = 0.0;
const float c_one = 1.0;
const float c_two = 2.0;
const vec4 c_vec4_one = vec4(c_one);
const float c_wrapThreshold = 0.9;

void main()
{
    float coordY = mix(gl_TexCoord[0].y,gl_TexCoord[0].x,u_usePalette);
    float coordX = mix(gl_TexCoord[0].x,gl_TexCoord[0].y,u_usePalette);
    float period = floor(coordY/u_npotEmulationFactor);
    coordX += u_npotEmulationXOffset*floor(mod(coordY,u_npotEmulationFactor));
    coordY = period+mod(coordY,u_npotEmulationFactor);
    vec2 newCoord = mix(gl_TexCoord[0].xy,mix(vec2(coordX,coordY),vec2(coordY,coordX),u_usePalette),u_npotEmulation);
#ifdef GL_ARB_shader_texture_lod
    vec2 texCoord = fract(newCoord.xy);
    texCoord = clamp(u_texturePosSize.zw*texCoord, u_halfTexelSize, u_texturePosSize.zw-u_halfTexelSize);
    vec4 color = texture2DGradARB(s_texture, u_texturePosSize.xy+texCoord, dFdx(texCoord), dFdy(texCoord));
#else
    vec2 transitionBlend = fwidth(floor(newCoord.xy));
    transitionBlend = fwidth(transitionBlend)+transitionBlend;
    vec2 texCoord = mix(fract(newCoord.xy), abs(c_one-mod(newCoord.xy+c_one, c_two)), transitionBlend);
    texCoord = clamp(u_texturePosSize.zw*texCoord, u_halfTexelSize, u_texturePosSize.zw-u_halfTexelSize);
    vec4 color = texture2D(s_texture, u_texturePosSize.xy+texCoord);
#endif

    float shade = clamp((u_shade+max(u_visFactor*v_distance-0.5*u_shadeInterpolate,c_zero)), c_zero, u_numShades-c_one);
    float shadeFrac = mod(shade, c_one);
    float colorIndex = texture2D(s_palswap, u_palswapPos+u_palswapSize*vec2(color.r, floor(shade)/u_numShades)).r;
    colorIndex = c_basepalOffset + c_basepalScale*colorIndex;
    vec4 palettedColor = texture2D(s_palette, vec2(colorIndex, c_zero));
    colorIndex = texture2D(s_palswap, u_palswapPos+u_palswapSize*vec2(color.r, (floor(shade)+c_one)/u_numShades)).r;
    colorIndex = c_basepalOffset + c_basepalScale*colorIndex;
    vec4 palettedColorNext = texture2D(s_palette, vec2(colorIndex, c_zero));
    palettedColor.rgb = mix(palettedColor.rgb, palettedColorNext.rgb, shadeFrac*u_shadeInterpolate);
    float fullbright = mix(u_usePalette*palettedColor.a, c_zero, u_useColorOnly);
    palettedColor.a = c_one-floor(color.r);
    color = mix(color, palettedColor, u_usePalette);

#ifdef POLYMOST1_EXTENDED
    vec4 detailColor = texture2D(s_detail, gl_TexCoord[3].xy);
    detailColor = mix(c_vec4_one, 2.0*detailColor, u_useDetailMapping*detailColor.a);
    color.rgb *= detailColor.rgb;
#endif

    color = mix(color, c_vec4_one, u_useColorOnly);

    // DEBUG
    //color = texture2D(s_palswap, gl_TexCoord[0].xy);
    //color = texture2D(s_palette, gl_TexCoord[0].xy);
    //color = texture2D(s_texture, gl_TexCoord[0].yx);

    color.rgb = mix(v_color.rgb*color.rgb, color.rgb, fullbright);

    float fogEnabled = mix(u_fogEnabled, c_zero, u_usePalette);
    fullbright = max(c_one-fogEnabled, fullbright);
    float fogFactor = clamp((gl_Fog.end-gl_FogFragCoord)*gl_Fog.scale, fullbright, c_one);
    //float fogFactor = clamp(gl_FogFragCoord, fullbright, c_one);
    color.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor);

#ifdef POLYMOST1_EXTENDED
    vec4 glowColor = texture2D(s_glow, gl_TexCoord[4].xy);
    color.rgb = mix(color.rgb, glowColor.rgb, u_useGlowMapping*glowColor.a*(c_one-u_useColorOnly));
#endif

    color.a *= v_color.a;

    gl_FragData[0] = color;
}
