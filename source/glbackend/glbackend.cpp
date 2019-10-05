#include <memory>
#include "glbackend.h"
#include "glad/glad.h"
#include "gl_samplers.h"
#include "gl_shader.h"

#include "baselayer.h"
#include "resourcefile.h"

std::unique_ptr<FResourceFile> engine_res;

// The resourge manager in cache1d is far too broken to add some arbitrary file without some adjustment.
// For now, keep this file here, until the resource management can be redone in a more workable fashion.
extern FString progdir;

void InitBaseRes()
{
	if (!engine_res)
	{
		// If we get here for the first time, load the engine-internal data.
		FString baseres = progdir + "demolition.pk3";
		engine_res.reset(FResourceFile::OpenResourceFile(baseres, true, true));
		if (!engine_res)
		{
			wm_msgbox("Fatal error", "Engine resources (demolition.pk3) not found");
			exit(-1); 
		}
	}
}

FileReader GetBaseResource(const char* fn)
{
	auto lump = engine_res->FindLump(fn);
	if (!lump)
	{
		wm_msgbox("Fatal error", "Base resource '%s' not found", fn);
		exit(-1);
	}
	return lump->NewReader();
}

GLInstance GLInterface;

void GLInstance::Init()
{
	InitBaseRes();
	if (!mSamplers)
	{
		mSamplers = new FSamplerManager;
		memset(LastBoundTextures, 0, sizeof(LastBoundTextures));
	}

	glinfo.vendor = (const char*)glGetString(GL_VENDOR);
	glinfo.renderer = (const char*)glGetString(GL_RENDERER);
	glinfo.version = (const char*)glGetString(GL_VERSION);
	glinfo.extensions = (const char*)glGetString(GL_EXTENSIONS);
	glinfo.bufferstorage = !!strstr(glinfo.extensions, "GL_ARB_buffer_storage");
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);
	if (!glinfo.dumped)
	{
		osdcmd_glinfo(NULL);
		glinfo.dumped = 1;
	}
	new(&renderState) PolymostRenderState;	// reset to defaults.
	LoadSurfaceShader();
	LoadVPXShader();
	LoadPolymostShader();

}

void GLInstance::LoadPolymostShader()
{
	auto fr1 = GetBaseResource("demolition/shaders/glsl/polymost.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetBaseResource("demolition/shaders/glsl/polymost.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	polymostShader = new PolymostShader();
	if (!polymostShader->Load("PolymostShader", (const char*)Vert.Data(), (const char*)Frag.Data()))
	{
		exit(1);
	}
	SetPolymostShader();
}

void GLInstance::LoadVPXShader()
{
	auto fr1 = GetBaseResource("demolition/shaders/glsl/animvpx.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetBaseResource("demolition/shaders/glsl/animvpx.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	vpxShader = new FShader();
	if (!vpxShader->Load("VPXShader", (const char*)Vert.Data(), (const char*)Frag.Data()))
	{
		exit(1);
	}
}

void GLInstance::LoadSurfaceShader()
{
	auto fr1 = GetBaseResource("demolition/shaders/glsl/surface.vp");
	TArray<uint8_t> Vert = fr1.Read();
	fr1 = GetBaseResource("demolition/shaders/glsl/surface.fp");
	TArray<uint8_t> Frag = fr1.Read();
	// Zero-terminate both strings.
	Vert.Push(0);
	Frag.Push(0);
	surfaceShader = new SurfaceShader();
	if (!surfaceShader->Load("SurfaceShader", (const char*)Vert.Data(), (const char*)Frag.Data()))
	{
		exit(1);
	}
}


void GLInstance::InitGLState(int fogmode, int multisample)
{
	glShadeModel(GL_SMOOTH);  // GL_FLAT
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glDisable(GL_DITHER);
	glEnable(GL_TEXTURE_2D);
    glHint(GL_FOG_HINT, GL_NICEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_DEPTH_CLAMP);

    if (multisample > 0 )
    {
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
        glEnable(GL_MULTISAMPLE);
    }
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
}

void GLInstance::Deinit()
{
	if (mSamplers) delete mSamplers;
	mSamplers = nullptr;
	if (polymostShader) delete polymostShader;
	polymostShader = nullptr;

	activeShader = nullptr;
}
	
std::pair<size_t, BaseVertex *> GLInstance::AllocVertices(size_t num)
{
	Buffer.resize(num);
	return std::make_pair((size_t)0, Buffer.data());
}

static GLint primtypes[] = 
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_QUADS,
	GL_LINES
};
	
void GLInstance::Draw(EDrawType type, size_t start, size_t count)
{
	if (activeShader == polymostShader) renderState.Apply(polymostShader);
	glBegin(primtypes[type]);
	auto p = &Buffer[start];
	for (size_t i = 0; i < count; i++, p++)
	{
		glTexCoord2f(p->u, p->v);
		glVertex3f(p->x, p->y, p->z);
	}
	glEnd();
}

int GLInstance::GetTextureID()
{
	// Generating large numbers of texture IDs piece by piece does not work well on modern NVidia drivers.

	if (currentindex == THCACHESIZE)
	{
		currentindex = 0;
		glGenTextures(THCACHESIZE, TextureHandleCache);
	}
	else currentindex++;
	return TextureHandleCache[currentindex];
}

FHardwareTexture* GLInstance::NewTexture()
{
	return new FHardwareTexture;
}

void GLInstance::BindTexture(int texunit, FHardwareTexture *tex, int sampler)
{
	if (!tex) return;
	if (texunit != 0) glActiveTexture(GL_TEXTURE0 + texunit);
	glBindTexture(GL_TEXTURE_2D, tex->GetTextureHandle());
	mSamplers->Bind(texunit, sampler == NoSampler? tex->GetSampler() : sampler, 0);
	if (texunit != 0) glActiveTexture(GL_TEXTURE0);
	LastBoundTextures[texunit] = tex->GetTextureHandle();
}

void GLInstance::UnbindTexture(int texunit)
{
	if (LastBoundTextures[texunit] != 0)
	{
		if (texunit != 0) glActiveTexture(GL_TEXTURE0+texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		if (texunit != 0) glActiveTexture(GL_TEXTURE0);
		LastBoundTextures[texunit] = 0;
	}
}

void GLInstance::UnbindAllTextures()
{
	for(int texunit = 0; texunit < MAX_TEXTURES; texunit++)
	{
		UnbindTexture(texunit);
	}
}

void GLInstance::EnableBlend(bool on)
{
	if (on) glEnable (GL_BLEND);
	else glDisable (GL_BLEND);
}

void GLInstance::EnableAlphaTest(bool on)
{
	if (on) glEnable (GL_ALPHA_TEST);
	else glDisable (GL_ALPHA_TEST);
}

void GLInstance::EnableDepthTest(bool on)
{
	if (on) glEnable (GL_DEPTH_TEST);
	else glDisable (GL_DEPTH_TEST);
}

void GLInstance::SetMatrix(int num, const VSMatrix *mat)
{
	matrices[num] = *mat;
	switch(num)
	{
		default:
			return;
		case Matrix_View:
			polymostShader->RotMatrix.Set(mat->get());
			break;

		case Matrix_Projection:
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(mat->get());
			break;
			
		case Matrix_ModelView:
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(mat->get());
			break;
			
		case Matrix_Texture3:
		case Matrix_Texture4:
			glActiveTexture(GL_TEXTURE3 + num - Matrix_Texture3);
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf(mat->get());
			glActiveTexture(GL_TEXTURE0);
			break;
	}
}

void GLInstance::EnableStencilWrite(int value)
{
    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, value, 0xFF);
}

void GLInstance::EnableStencilTest(int value)
{
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, value, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void GLInstance::DisableStencil()
{
	glDisable(GL_STENCIL_TEST);
}

void GLInstance::SetCull(int type, int winding)
{
	if (type == Cull_None)
	{
		glDisable(GL_CULL_FACE);
	}
	else if (type == Cull_Front)
	{
		glFrontFace(winding == Winding_CW ? GL_CW : GL_CCW);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else if (type == Cull_Back)
	{
		glFrontFace(winding == Winding_CW ? GL_CW : GL_CCW);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
}

void GLInstance::SetColor(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

void GLInstance::SetDepthFunc(int func)
{
	int f[] = { GL_ALWAYS, GL_LESS, GL_EQUAL, GL_LEQUAL };
	glDepthFunc(f[func]);
}

void GLInstance::SetFogLinear(float* color, float start, float end)
{
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, start);
	glFogf(GL_FOG_END, end);
	glFogfv(GL_FOG_COLOR, color);
}

void GLInstance::SetFogExp2(float* color, float coefficient)
{
	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogf(GL_FOG_DENSITY, coefficient);
	glFogfv(GL_FOG_COLOR, color);
}

void GLInstance::SetColorMask(bool on)
{
	glColorMask(on, on, on, on);
}

void GLInstance::SetDepthMask(bool on)
{
	glDepthMask(on);
}

static int blendstyles[] = { GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA };

void GLInstance::SetBlendFunc(int src, int dst)
{
	glBlendFunc(blendstyles[src], blendstyles[dst]);
}

static int renderops[] = { GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT };

void GLInstance::SetBlendOp(int op)
{
	glBlendEquation(renderops[op]);
}

void GLInstance::ClearScreen(float r, float g, float b, bool depth)
{
	glClearColor(r, g, b, 1.f);
	glClear(depth ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT);
}

void GLInstance::ClearDepth()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void GLInstance::SetAlphaThreshold(float al)
{
	glAlphaFunc(GL_GREATER, al);
}

void GLInstance::SetViewport(int x, int y, int w, int h)
{
	glViewport(x, y, w, h);
}

void GLInstance::SetWireframe(bool on)
{
	glPolygonMode(GL_FRONT_AND_BACK,on? GL_LINE : GL_FILL); 
}

void GLInstance::ReadPixels(int xdim, int ydim, uint8_t* buffer)
{
	glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buffer);
}

void GLInstance::SetPolymostShader()
{
	if (activeShader != polymostShader)
	{
		polymostShader->Bind();
		activeShader = polymostShader;
	}
	//GLInterface.BindTexture(1, palswapTextureID);
	//GLInterface.BindTexture(2, paletteTextureIDs[curbasepal]);

}

void GLInstance::SetSurfaceShader()
{
	if (activeShader != surfaceShader)
	{
		surfaceShader->Bind();
		activeShader = surfaceShader;
	}
}

void GLInstance::SetVPXShader()
{
	if (activeShader != vpxShader)
	{
		vpxShader->Bind();
		activeShader = vpxShader;
	}
}


void PolymostRenderState::Apply(PolymostShader* shader)
{
	shader->Clamp.Set(Clamp);
	shader->Shade.Set(Shade);
	shader->NumShades.Set(NumShades);
	shader->VisFactor.Set(VisFactor);
	shader->FogEnabled.Set(FogEnabled);
	shader->UseColorOnly.Set(UseColorOnly);
	shader->UsePalette.Set(UsePalette);
	shader->UseDetailMapping.Set(UseDetailMapping);
	shader->UseGlowMapping.Set(UseGlowMapping);
	shader->NPOTEmulation.Set(NPOTEmulation);
	shader->NPOTEmulationFactor.Set(NPOTEmulationFactor);
	shader->NPOTEmulationXOffset.Set(NPOTEmulationXOffset);
	shader->ShadeInterpolate.Set(ShadeInterpolate);
	shader->Brightness.Set(Brightness);
}

#if 0

static void polymost_setPalswap(uint32_t index)
{
	static uint32_t lastPalswapIndex;

	if (currentShaderProgramID != polymost1CurrentShaderProgramID)
		return;

	lastPalswapIndex = index;
	polymost1PalswapPos.x = index * polymost1PalswapSize.x;
	polymost1PalswapPos.y = floorf(polymost1PalswapPos.x);
	polymost1PalswapPos = { polymost1PalswapPos.x - polymost1PalswapPos.y + (0.5f / PALSWAP_TEXTURE_SIZE),
							polymost1PalswapPos.y * polymost1PalswapSize.y + (0.5f / PALSWAP_TEXTURE_SIZE) };
	glUniform2f(polymost1PalswapPosLoc, polymost1PalswapPos.x, polymost1PalswapPos.y);
}

static void polymost_setPalswapSize(uint32_t width, uint32_t height)
{
	if (currentShaderProgramID != polymost1CurrentShaderProgramID)
		return;

	polymost1PalswapSize = { width * (1.f / PALSWAP_TEXTURE_SIZE),
							 height * (1.f / PALSWAP_TEXTURE_SIZE) };

	polymost1PalswapInnerSize = { (width - 1) * (1.f / PALSWAP_TEXTURE_SIZE),
								  (height - 1) * (1.f / PALSWAP_TEXTURE_SIZE) };

	glUniform2f(polymost1PalswapSizeLoc, polymost1PalswapInnerSize.x, polymost1PalswapInnerSize.y);
}


#endif