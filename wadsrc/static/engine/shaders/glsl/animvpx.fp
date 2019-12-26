// YUV->RGB conversion fragment shader adapted from
// http://www.fourcc.org/fccyvrgb.php: Want some sample code?
// direct link: http://www.fourcc.org/source/YUV420P-OpenGL-GLSLang.c
#version 330

uniform sampler2D tex;
in vec2 v_texCoord;
out vec4 FragColor;

void main(void) {

  float r,g,b,y,u,v;
  vec3 yuv;

  yuv = texture2D(tex, v_texCoord.st).rgb;
  y = yuv.r;
  u = yuv.g;
  v = yuv.b;

  y = 1.1643*(y-0.0625);
  u = u-0.5;
  v = v-0.5;

  r = y + 1.5958*v;
  g = y - 0.39173*u - 0.81290*v;
  b = y + 2.017*u;

  FragColor = vec4(r,g,b,1.0);
};
