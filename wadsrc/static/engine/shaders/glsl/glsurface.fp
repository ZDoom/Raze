#version 330

//s_texture points to an indexed color texture
uniform sampler2D s_texture;
//s_palette is the palette texture
uniform sampler2D s_palette;

in vec2 v_texCoord;
out vec4 FragColor;

const float c_paletteScale = 255.0/256.0;
const float c_paletteOffset = 0.5/256.0;

void main()
{
 vec4 color = texture2D(s_texture, v_texCoord.xy);
 color.r = c_paletteOffset + c_paletteScale*color.r;
 color.rgb = texture2D(s_palette, color.rg).rgb;
 
 FragColor = color;
}
