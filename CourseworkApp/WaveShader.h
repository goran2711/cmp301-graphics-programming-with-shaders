// Shader that renders tessellated geometry (3 control points), and additionally passes a time variable to the shader
// Extends LightingShadowShader so that shadow mapping can be used on the tessellated geometry

#pragma once
#include "LightingShadowShader.h"
#include "ShaderBuffers.h"

class Camera;

class WaveShader : public LightingShadowShader
{
public:
	using CameraBufferType = BufferType::CameraBufferType;
	using TessellationPropertiesType = BufferType::TessellationPropertiesType;

	static constexpr int DEFAULT_MIN_TESS = 0;
	static constexpr int DEFAULT_MAX_TESS = 3;
	static constexpr float DEFAULT_MIN_TESS_DISTANCE = 32.f;
	static constexpr float DEFAULT_MAX_TESS_DISTANCE = 1.f;

	WaveShader(ID3D11Device* device, ID3D11DeviceContext * context, HWND hwnd);
	WaveShader(const WaveShader&) = delete;
	WaveShader& operator=(const WaveShader&) = delete;
	~WaveShader();

	void render(ID3D11DeviceContext* context, int vertexCount) override;

	void setTessellationProperties(ID3D11DeviceContext* context, int minTess, int maxTess, float minDistance, float maxDistance);
	void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, CXMMATRIX shadowTransform, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMap, float time);

protected:
	void initShader(WCHAR*, WCHAR*) override;

	ID3D11Buffer* mTimeBuffer = nullptr;
	ID3D11Buffer* mTessellationProperties = nullptr;
};

