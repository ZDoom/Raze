/*
 * glsurface.cpp
 *  A 32-bit rendering surface that can quickly blit 8-bit paletted buffers implemented in OpenGL.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#include "glsurface.h"
#include "glad/glad.h"

#include "baselayer.h"
#include "build.h"
#include "../../glbackend/glbackend.h"

static void* buffer;
static FHardwareTexture* bufferTexture;
static vec2_t bufferRes;

static FHardwareTexture* paletteTexture;

static GLuint shaderProgramID = 0;
static GLint texSamplerLoc = -1;
static GLint paletteSamplerLoc = -1;

static GLuint compileShader(GLenum shaderType, const char* const source)
{
    GLuint shaderID = glCreateShader(shaderType);
    if (shaderID == 0)
        return 0;

    const char* const sources[1] = {source};
    glShaderSource(shaderID,
                   1,
                   sources,
                   NULL);
    glCompileShader(shaderID);

    GLint compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus)
    {
        GLint logLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
        OSD_Printf("Compile Status: %u\n", compileStatus);
        if (logLength > 0)
        {
            char *infoLog = (char*) Xmalloc(logLength);
            glGetShaderInfoLog(shaderID, logLength, &logLength, infoLog);
            OSD_Printf("Log:\n%s\n", infoLog);
            Xfree(infoLog);
        }
    }

    return shaderID;
}

bool glsurface_initialize(vec2_t bufferResolution)
{
    if (buffer)
        glsurface_destroy();

    bufferRes = bufferResolution;
    buffer    = Xaligned_alloc(16, bufferRes.x * bufferRes.y);

	bufferTexture = GLInterface.NewTexture();
	bufferTexture->CreateTexture(bufferRes.x, bufferRes.y, true, false);

    glsurface_setPalette(curpalettefaded);

    const char* const VERTEX_SHADER_CODE =
        "#version 110\n\
         \n\
         attribute vec4 i_vertPos;\n\
         attribute vec2 i_texCoord;\n\
         \n\
         varying vec2 v_texCoord;\n\
         \n\
         void main()\n\
         {\n\
             gl_Position = i_vertPos;\n\
             v_texCoord = i_texCoord;\n\
         }\n";
    const char* const FRAGMENT_SHADER_CODE =
        "#version 110\n\
         \n\
         //s_texture points to an indexed color texture\n\
         uniform sampler2D s_texture;\n\
         //s_palette is the palette texture\n\
         uniform sampler2D s_palette;\n\
         \n\
         varying vec2 v_texCoord;\n\
         \n\
         const float c_paletteScale = 255.0/256.0;\n\
         const float c_paletteOffset = 0.5/256.0;\n\
         \n\
         void main()\n\
         {\n\
             vec4 color = texture2D(s_texture, v_texCoord.xy);\n\
             color.r = c_paletteOffset + c_paletteScale*color.r;\n\
             color.rgb = texture2D(s_palette, color.rg).rgb;\n\
             \n\
             // DEBUG \n\
             //color = texture2D(s_palette, v_texCoord.xy);\n\
             //color = texture2D(s_texture, v_texCoord.xy);\n\
             \n\
             gl_FragColor = color;\n\
         }\n";

    shaderProgramID = glCreateProgram();
    GLuint vertexShaderID = compileShader(GL_VERTEX_SHADER, VERTEX_SHADER_CODE);
    GLuint fragmentShaderID = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER_CODE);
    glAttachShader(shaderProgramID, vertexShaderID);
    glAttachShader(shaderProgramID, fragmentShaderID);
    glBindAttribLocation(shaderProgramID, 0, "i_vertPos");
    glBindAttribLocation(shaderProgramID, 1, "i_texCoord");
    glLinkProgram(shaderProgramID);
    glDetachShader(shaderProgramID, vertexShaderID);
    glDeleteShader(vertexShaderID);
    glDetachShader(shaderProgramID, fragmentShaderID);
    glDeleteShader(fragmentShaderID);
    glUseProgram(shaderProgramID);

    texSamplerLoc = glGetUniformLocation(shaderProgramID, "s_texture");
    paletteSamplerLoc = glGetUniformLocation(shaderProgramID, "s_palette");

    glUniform1i(texSamplerLoc, 0);
    glUniform1i(paletteSamplerLoc, 1);

    return true;
}

void glsurface_destroy()
{
    if (!buffer)
        return;

    ALIGNED_FREE_AND_NULL(buffer);

	delete bufferTexture;
	bufferTexture = nullptr;
	delete paletteTexture;
	paletteTexture = nullptr;

    glUseProgram(0);
    glDeleteProgram(shaderProgramID);
    shaderProgramID = 0;
}

void glsurface_setPalette(void* pPalette)
{
    if (!buffer)
        return;
    if (!pPalette)
        return;

	if (!paletteTexture)
	{
		paletteTexture = GLInterface.NewTexture();
		paletteTexture->CreateTexture(256, 1, false, false);
	}
	paletteTexture->LoadTexture(palette);
	GLInterface.BindTexture(1, paletteTexture, Sampler2DNoFilter);
}

void* glsurface_getBuffer()
{
    return buffer;
}

vec2_t glsurface_getBufferResolution()
{
    return bufferRes;
}

void glsurface_blitBuffer()
{
    if (!buffer)
        return;

	bufferTexture->LoadTexture((uint8_t*)buffer);
	GLInterface.BindTexture(0, bufferTexture, Sampler2DNoFilter);

	auto data = GLInterface.AllocVertices(4);
	auto vt = data.second;

	vt[0].Set(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f); //top-left
	vt[1].Set(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f); //bottom-left
	vt[2].Set(1.0f, 1.0f, 0.0f, 1.0f, 0.0f); //top-right
	vt[3].Set(1.0f, -1.0f, 0.0f, 1.0f, 1.0f);  //bottom-right
	GLInterface.Draw(DT_TRIANGLE_STRIP, data.first, 4);
}
