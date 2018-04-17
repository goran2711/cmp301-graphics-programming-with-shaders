// Render texture object
// Is a texture object that can be used as an alternative render target. Store what is rendered to it.
// Size can be speicified but traditionally this will match window size.
// Used in post processing and multi-render stages.

#ifndef _RENDERTEXTURE_H_
#define _RENDERTEXTURE_H_

#include <d3d11.h>
#include <directxmath.h>

using namespace DirectX;

class RenderTexture
{
public:
	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	RenderTexture(ID3D11Device* device, int textureWidth, int textureHeight, float screenNear, float screenDepth);
	~RenderTexture();

	void setRenderTarget(ID3D11DeviceContext* deviceContext);
	void clearRenderTarget(ID3D11DeviceContext* deviceContext, float red, float green, float blue, float alpha);

	void setColourWrites(ID3D11DeviceContext* context, bool val);

	ID3D11ShaderResourceView* getShaderResourceView() const;
	ID3D11ShaderResourceView* getDepthShaderResourceView() const;
	ID3D11UnorderedAccessView* getUnorderedAccessView() const;

	XMMATRIX getProjectionMatrix() const;
	XMMATRIX getOrthoMatrix() const;

	int getTextureWidth() const;
	int getTextureHeight() const;

private:
	int textureWidth, textureHeight;
	ID3D11Texture2D* renderTargetTexture;
	ID3D11RenderTargetView* renderTargetView;
	ID3D11ShaderResourceView* shaderResourceView;
	ID3D11UnorderedAccessView* unorderedAccessView;
	ID3D11ShaderResourceView* depthShaderResourceView;
	ID3D11Texture2D* depthStencilBuffer;
	ID3D11DepthStencilView* depthStencilView;
	D3D11_VIEWPORT viewport;

	ID3D11BlendState* noColourWrites;
	ID3D11BlendState* previousBlendState;
	float previousBlendFactor[4];
	UINT previousSampleMask;

	XMMATRIX projectionMatrix;
	XMMATRIX orthoMatrix;
};

#endif