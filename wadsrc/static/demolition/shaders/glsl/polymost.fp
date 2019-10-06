#version 120

#extension GL_ARB_uniform_buffer_object:enable

//s_texture points to an indexed color texture
uniform sampler2D s_texture;
//s_palswap is the palette swap texture where u is the color index and v is the shade
uniform sampler2D s_palswap;
//s_palette is the base palette texture where u is the color index
uniform sampler2D s_palette;

uniform sampler2D s_detail;
uniform sampler2D s_glow;

uniform vec2 u_palswapPos;
uniform vec2 u_palswapSize;

uniform vec2 u_clamp;

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
uniform float u_brightness;

uniform float u_useDetailMapping;
uniform float u_useGlowMapping;

uniform int u_tinteffect;
uniform vec3 u_tintcolor;

varying vec4 v_color;
varying float v_distance;
varying vec4 v_texCoord;
varying vec4 v_detailCoord;
varying vec4 v_glowCoord;
varying float v_fogCoord;

const float c_basepalScale = 255.0/256.0;
const float c_basepalOffset = 0.5/256.0;

const float c_zero = 0.0;
const float c_one = 1.0;
const float c_two = 2.0;
const vec4 c_vec4_one = vec4(c_one);
const float c_wrapThreshold = 0.9;

layout(std140) uniform Palette {
	vec4 palette[256];
};

layout(std140) uniform Palswap {
	int palswap[256];
};

//===========================================================================
//
// Color to grayscale
//
//===========================================================================

float grayscale(vec4 color)
{
	return dot(color.rgb, vec3(0.3, 0.56, 0.14));
}
 
//===========================================================================
//
// Hightile tinting code. (hictinting[dapalnum]) This can be done inside the shader 
// to avoid costly texture duplication (but needs a more modern GLSL than 1.10.)
//
//===========================================================================

vec4 convertColor(vec4 color, int effect, vec3 tint)
{
#if 0

	if (effect & HICTINT_GRAYSCALE)
	{
		float g = grayscale(color);
		color = vec4(g, g, g, color.a);
	}

	if (effect & HICTINT_INVERT)
	{
		color = vec4(1.0 - color.r, 1.0 - color.g, 1.0 - color.b);
	}
	
	vec3 tcol = color.rgb * 255.0;	// * 255.0 to make it easier to reuse the integer math.
	tint *= 255.0;

	if (effect & HICTINT_COLORIZE)
	{
		tcol.b = min(((tcol.b) * tint.r) / 64.0, 255.0);
		tcol.g = min(((tcol.g) * tint.g) / 64.0, 255.0);
		tcol.r = min(((tcol.r) * tint.b) / 64.0, 255.0);
	}

	switch (effect & HICTINT_BLENDMASK)
	{
		case HICTINT_BLEND_SCREEN:
			tcol.b = 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 256.0);
			tcol.g = 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 256.0);
			tcol.r = 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 256.0);
			break;
		case HICTINT_BLEND_OVERLAY:
			tcol.b = tcol.b < 128.0? (tcol.b * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - tint.r)) / 128.0);
			tcol.g = tcol.g < 128.0? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - tint.g)) / 128.0);
			tcol.r = tcol.r < 128.0? (tcol.r * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - tint.b)) / 128.0);
			break;
		case HICTINT_BLEND_HARDLIGHT:
			tcol.b = tint.r < 128.0 ? (tcol.b * tint.r) / 128.0 : 255.0 - (((255.0 - tcol.b) * (255.0 - r)) / 128.0);
			tcol.g = tint.g < 128.0 ? (tcol.g * tint.g) / 128.0 : 255.0 - (((255.0 - tcol.g) * (255.0 - g)) / 128.0);
			tcol.r = tint.b < 128.0 ? (tcol.r * tint.b) / 128.0 : 255.0 - (((255.0 - tcol.r) * (255.0 - b)) / 128.0);
			break;
	}
	color.rgb = tcol / 255.0;
#endif
	return color;
}

//===========================================================================
//
// Talk about all the wrong way of being 'efficient'... :(
//
//===========================================================================

void main()
{
    float coordY = mix(v_texCoord.y,v_texCoord.x,u_usePalette);
    float coordX = mix(v_texCoord.x,v_texCoord.y,u_usePalette);
    float period = floor(coordY/u_npotEmulationFactor);
    coordX += u_npotEmulationXOffset*floor(mod(coordY,u_npotEmulationFactor));
    coordY = period+mod(coordY,u_npotEmulationFactor);
    vec2 newCoord = mix(v_texCoord.xy,mix(vec2(coordX,coordY),vec2(coordY,coordX),u_usePalette),u_npotEmulation);
    vec2 transitionBlend = fwidth(floor(newCoord.xy));
    transitionBlend = fwidth(transitionBlend)+transitionBlend;
    vec2 texCoord = mix(mix(fract(newCoord.xy), abs(c_one-mod(newCoord.xy+c_one, c_two)), transitionBlend), clamp(newCoord.xy, c_zero, c_one), u_clamp);
    vec4 color = texture2D(s_texture, texCoord);

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

	if (u_useDetailMapping != 0.0)
	{
		vec4 detailColor = texture2D(s_detail, v_detailCoord.xy);
		detailColor = mix(c_vec4_one, 2.0*detailColor, detailColor.a);
		color.rgb *= detailColor.rgb;
	}

    color = mix(color, c_vec4_one, u_useColorOnly);

    color.rgb = mix(v_color.rgb*color.rgb, color.rgb, fullbright);

    float fogEnabled = mix(u_fogEnabled, c_zero, u_usePalette);
    fullbright = max(c_one-fogEnabled, fullbright);
    float fogFactor = clamp((gl_Fog.end-v_fogCoord)*gl_Fog.scale, fullbright, c_one);
    //float fogFactor = clamp(v_fogCoord, fullbright, c_one);
    color.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor);

	if (u_useGlowMapping != 0.0)
	{
		vec4 glowColor = texture2D(s_glow, v_glowCoord.xy);
		color.rgb = mix(color.rgb, glowColor.rgb, glowColor.a*(c_one-u_useColorOnly));
	}
	
    color.a *= v_color.a;
    color.rgb = pow(color.rgb, vec3(u_brightness));

    gl_FragData[0] = color;
}
