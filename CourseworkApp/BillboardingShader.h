// Shader that converts a point mesh to a billboarded quad via the geometry shader

#pragma once
#include "..\DXFramework\BaseShader.h"
#include "ShaderBuffers.h"

class Camera;

class BillboardingShader : public BaseShader
{
public:
	using CameraBufferType = BufferType::CameraBufferType;

	BillboardingShader(ID3D11Device* device, HWND hwnd);
	BillboardingShader(const BillboardingShader&) = delete;
	BillboardingShader& operator=(const BillboardingShader&) = delete;
	~BillboardingShader();

	void render(ID3D11DeviceContext * context, int vertexCount) override;

	void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, ID3D11ShaderResourceView* texture, Camera* camera);

protected:
	void initShader(WCHAR*, WCHAR*) override;

	ID3D11Buffer* mCameraBuffer;
};

