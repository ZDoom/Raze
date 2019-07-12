#version 110

varying vec4 v_color;
varying float v_distance;

//u_texturePosSize is the texture position & size packaged into a single vec4 as {pos.x, pos.y, size.x, size.y}
uniform vec4 u_texturePosSize;
uniform float u_usePalette;
uniform mat4 u_rotMatrix;

const float c_zero = 0.0;
const float c_one  = 1.0;

void main()
{
   vec4 vertex = u_rotMatrix * gl_Vertex;
   vec4 eyeCoordPosition = gl_ModelViewMatrix * vertex;
   gl_Position = gl_ModelViewProjectionMatrix * vertex;

   eyeCoordPosition.xyz /= eyeCoordPosition.w;

   gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
   gl_TexCoord[0] = mix(gl_TexCoord[0].xyzw, gl_TexCoord[0].yxzw, u_usePalette);

   gl_TexCoord[3] = gl_TextureMatrix[3] * gl_MultiTexCoord3;
   gl_TexCoord[4] = gl_TextureMatrix[4] * gl_MultiTexCoord4;

   gl_FogFragCoord = abs(eyeCoordPosition.z);
   //gl_FogFragCoord = clamp((gl_Fog.end-abs(eyeCoordPosition.z))*gl_Fog.scale, c_zero, c_one);

   v_color = gl_Color;
   v_distance = gl_Vertex.z;
}
