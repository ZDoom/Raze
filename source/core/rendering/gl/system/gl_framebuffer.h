#ifndef __GL_FRAMEBUFFER
#define __GL_FRAMEBUFFER

#include "gl_sysfb.h"

#include <memory>

namespace OpenGLRenderer
{

class FGLDebug;

class OpenGLFrameBuffer : public SystemGLFrameBuffer
{
	typedef SystemGLFrameBuffer Super;

public:

	explicit OpenGLFrameBuffer() {}
	OpenGLFrameBuffer(void *hMonitor, bool fullscreen) ;
	~OpenGLFrameBuffer();

	void InitializeState() override;
	void Update() override;
	void Draw2D() override;

	const char* DeviceName() const override;
	void SetTextureFilterMode() override;
	IHardwareTexture* CreateHardwareTexture() override;
#ifdef IMPLEMENT_IT
	void PrecacheMaterial(FMaterial *mat, int translation) override;
	FModelRenderer *CreateModelRenderer(int mli) override;
	void TextureFilterChanged() override;
#endif
	void BeginFrame() override;
	//void SetViewportRects(IntRect *bounds) override;
	void BlurScene(float amount) override;
	IVertexBuffer *CreateVertexBuffer() override;
	IIndexBuffer *CreateIndexBuffer() override;
	IDataBuffer *CreateDataBuffer(int bindingpoint, bool ssbo, bool needsresize) override;

	// Retrieves a buffer containing image data for a screenshot.
	// Hint: Pitch can be negative for upside-down images, in which case buffer
	// points to the last row in the buffer, which will be the first row output.
	virtual TArray<uint8_t> GetScreenshotBuffer(int &pitch, ESSType &color_type, float &gamma) override;

	void Swap();
	bool IsHWGammaActive() const { return HWGammaActive; }

	void SetVSync(bool vsync) override;

	//void Draw2D() override;
	void PostProcessScene(int fixedcm, const std::function<void()> &afterBloomDrawEndScene2D) override;

	bool HWGammaActive = false;			// Are we using hardware or software gamma?
	std::shared_ptr<FGLDebug> mDebug;	// Debug API
    
	int camtexcount = 0;
};

}

#endif //__GL_FRAMEBUFFER
