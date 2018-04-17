// Extension of the LightingShader class, except this also does shadow mapping

#pragma once
#include "LightingShader.h"

class LightingShadowShader : public LightingShader
{
public:
	using MatrixBufferType = BufferType::MatrixBufferWithLightType;

	LightingShadowShader(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd, WCHAR* vs = L"lighting_shadow_vs.cso", WCHAR* ps = L"lighting_shadow_ps.cso");
	LightingShadowShader(const LightingShadowShader&) = delete;
	LightingShadowShader& operator=(const LightingShadowShader&) = delete;
	virtual ~LightingShadowShader();

	void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX proj, CXMMATRIX shadowTransform, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMap);

	virtual void render(ID3D11DeviceContext* context, int vertexCount) override;

protected:
	virtual void initShader(WCHAR* vsFilename, WCHAR* psFilename) override;

	// Comparison sampler for sampling the shadow map
	ID3D11SamplerState* shadowMapSampler = nullptr;
};

