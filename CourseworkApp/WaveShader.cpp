#include "WaveShader.h"
#include "..\DXFramework\Camera.h"
#include "Utility.h"

WaveShader::WaveShader(ID3D11Device * device, ID3D11DeviceContext * context, HWND hwnd)
		// Parent class initialises matrix-, light-, camera- and fog buffers
	:	LightingShadowShader(device, context, hwnd, L"passthrough_vs.cso", L"lighting_shadow_ps.cso")
{
	// Initialise hull and domain shader
	initShader(NULL, NULL);
	setTessellationProperties(context, DEFAULT_MIN_TESS, DEFAULT_MAX_TESS, DEFAULT_MIN_TESS_DISTANCE, DEFAULT_MAX_TESS_DISTANCE);
}

WaveShader::~WaveShader()
{
	mTimeBuffer->Release();
	mTessellationProperties->Release();
}

void WaveShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	ID3D11SamplerState* samplers[2] = { sampleState, shadowMapSampler };
	context->PSSetSamplers(0, 2, samplers);

	BaseShader::render(context, vertexCount);

	// Clean up
	UnsetPSShaderInputs(context);
	context->HSSetShader(NULL, NULL, 0);
	context->DSSetShader(NULL, NULL, 0);
}

void WaveShader::setTessellationProperties(ID3D11DeviceContext* context, int minTess, int maxTess, float minDistance, float maxDistance)
{
	D3D11_MAPPED_SUBRESOURCE map;
	context->Map(mTessellationProperties, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	TessellationPropertiesType* ptr = static_cast<TessellationPropertiesType*>(map.pData);
	ptr->gMinTess = minTess;
	ptr->gMaxTess = maxTess;
	ptr->gMinDistance = minDistance;
	ptr->gMaxDistance = maxDistance;

	context->Unmap(mTessellationProperties, 0);
}

void XM_CALLCONV WaveShader::setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, CXMMATRIX shadowTransform, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMap, float time)
{
	// NOTE: This also sets the matrix and camera buffer in the vertex shader, which we won't use,
	//       but at least it doesn't cause any issues so we can avoid a lot of duplicate code.
	LightingShadowShader::setShaderParameters(context, world, view, projection, shadowTransform, camera, texture, shadowMap);

	D3D11_MAPPED_SUBRESOURCE map;
	context->Map(mTimeBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	*static_cast<float*>(map.pData) = time;

	context->Unmap(mTimeBuffer, 0);

	// Set constant buffers
	ID3D11Buffer* hsBuffers[3] = { matrixBuffer, cameraBuffer, mTessellationProperties };
	context->HSSetConstantBuffers(0, 3, hsBuffers);

	ID3D11Buffer* dsBuffers[3] = { matrixBuffer, cameraBuffer, mTimeBuffer };
	context->DSSetConstantBuffers(0, 3, dsBuffers);
}

void WaveShader::initShader(WCHAR *, WCHAR *)
{
	loadHullShader(L"wave_hs.cso");
	loadDomainShader(L"wave_ds.cso");

	// Create time buffer
	D3D11_BUFFER_DESC timeDesc;
	ZeroMemory(&timeDesc, sizeof(D3D11_BUFFER_DESC));
	timeDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	timeDesc.Usage = D3D11_USAGE_DYNAMIC;
	timeDesc.ByteWidth = sizeof(float) * 4;
	timeDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	renderer->CreateBuffer(&timeDesc, 0, &mTimeBuffer);

	// Create tessellation properties buffer
	D3D11_BUFFER_DESC tessellationDesc;
	ZeroMemory(&tessellationDesc, sizeof(D3D11_BUFFER_DESC));
	tessellationDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	tessellationDesc.Usage = D3D11_USAGE_DYNAMIC;
	tessellationDesc.ByteWidth = sizeof(TessellationPropertiesType);
	tessellationDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	renderer->CreateBuffer(&tessellationDesc, 0, &mTessellationProperties);
}