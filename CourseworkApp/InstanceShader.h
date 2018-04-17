// Shader utilising hardware instancing to efficiently draw several instances of the same mesh

#pragma once
#include "LightingShader.h"
#include "ShaderBuffers.h"

// 16-byte aligned because it stores XMMATRIX
__declspec(align(16))
class InstanceShader : public LightingShader
{
public:
	static constexpr int MAX_INSTANCES = 100'000;
	using MatrixBufferType = BufferType::MatrixBufferWithoutWorldType;

	// Instance-unique data
	struct InstanceBufferType
	{
		XMMATRIX world;
	};

	InstanceShader(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
	InstanceShader(const InstanceShader&) = delete;
	InstanceShader& operator=(const InstanceShader&) = delete;
	~InstanceShader();

	// Store an object's transform matrix(instance data)
	void XM_CALLCONV addInstance(FXMMATRIX world);

	void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX view, CXMMATRIX projection, Camera* camera, ID3D11ShaderResourceView* texture);

	// Called once to render all instances added via InstanceShader::addInstance
	void render(ID3D11DeviceContext* context, int vertexCount) override;

protected:
	void initShader(WCHAR* vs);

	// InstanceShader needs a unique input layout, and does therefore not use the one provided by BaseShader
	void loadVertexShader(WCHAR* vs);

	ID3D11Buffer* instanceBuffer;
	
	int usedInstances = 0;
	InstanceBufferType instances[MAX_INSTANCES];
};

