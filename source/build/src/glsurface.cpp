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
static GLuint bufferTexID;
static vec2_t bufferRes;

static GLuint paletteTexID;

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
            Bfree(infoLog);
        }
    }

    return shaderID;
}

bool glsurface_initialize(vec2_t bufferResolution)
{
    if (buffer)
        glsurface_destroy();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    bufferRes = bufferResolution;
    buffer    = Xaligned_alloc(16, bufferRes.x * bufferRes.y);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &bufferTexID);
    glBindTexture(GL_TEXTURE_2D, bufferTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bufferRes.x, bufferRes.y, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

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

    glDeleteTextures(1, &bufferTexID);
    bufferTexID = 0;
    glDeleteTextures(1, &paletteTexID);
    paletteTexID = 0;

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

    glActiveTexture(GL_TEXTURE1);
    if (paletteTexID)
    {
        // assume the texture is already bound to GL_TEXTURE1
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void*) pPalette);
    }
    else
    {
        glGenTextures(1, &paletteTexID);
        glBindTexture(GL_TEXTURE_2D, paletteTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPalette);
    }
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

    glActiveTexture(GL_TEXTURE0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bufferRes.x, bufferRes.y, GL_RED, GL_UNSIGNED_BYTE, (void*) buffer);

	auto data = GLInterface.AllocVertices(4);
	auto vt = data.second;

	vt[0].Set(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f); //top-left
	vt[1].Set(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f); //bottom-left
	vt[2].Set(1.0f, 1.0f, 0.0f, 1.0f, 0.0f); //top-right
	vt[3].Set(1.0f, -1.0f, 0.0f, 1.0f, 1.0f);  //bottom-right
	GLInterface.Draw(DT_TRIANGLE_STRIP, data.first, 4);
}
