#include "LightingShadowShader.h"
#include "..\DXFramework\Light.h"
#include "..\DXFramework\Camera.h"
#include "Utility.h"

LightingShadowShader::LightingShadowShader(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd, WCHAR* vs, WCHAR* ps)
	: LightingShader(device, context, hwnd, vs, ps)
{
	// LightingShader calls its own initShader function first--loading the shaders and creating most of the resources 
	initShader(vs, ps);
}

LightingShadowShader::~LightingShadowShader()
{
	shadowMapSampler->Release();
}

void XM_CALLCONV LightingShadowShader::setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX proj, CXMMATRIX shadowTransform, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* shadowMap)
{
	D3D11_MAPPED_SUBRESOURCE map;
	context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	// Update matrix buffer
	MatrixBufferType* matrixPtr = static_cast<MatrixBufferType*>(map.pData);
	matrixPtr->world = world;
	matrixPtr->view = view;
	matrixPtr->projection = proj;

	matrixPtr->shadowTransform = shadowTransform;

	context->Unmap(matrixBuffer, 0);

	// Update light buffer
	context->Map(lightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	LightBufferType* lightPtr = static_cast<LightBufferType*>(map.pData);

	lightPtr->globalAmbient = mAmbient;
	
	memcpy_s(lightPtr->lights, sizeof(ShaderLight) * MAX_LIGHTS, lights, sizeof(ShaderLight) * MAX_LIGHTS);

	context->Unmap(lightBuffer, 0);

	// Update camera buffer
	context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	CameraBufferType* cameraPtr = static_cast<CameraBufferType*>(map.pData);
	ZeroMemory(cameraPtr, sizeof(CameraBufferType));
	cameraPtr->position = camera->getPosition();

	context->Unmap(cameraBuffer, 0);

	// 'Dispatch' constant buffer
	ID3D11Buffer* vsBuffers[2] = { matrixBuffer, cameraBuffer };
	context->VSSetConstantBuffers(0, 2, vsBuffers);

	ID3D11Buffer* psBuffers[2] = { lightBuffer, fogBuffer };
	context->PSSetConstantBuffers(0, 2, psBuffers);

	// Send textures
	ID3D11ShaderResourceView* textures[2] = { texture, shadowMap };
	context->PSSetShaderResources(0, 2, textures);
}

void LightingShadowShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	ID3D11SamplerState* samplers[2] = { sampleState, shadowMapSampler };
	context->PSSetSamplers(0, 2, samplers);

	BaseShader::render(context, vertexCount);
}

void LightingShadowShader::initShader(WCHAR* vsFilename, WCHAR* psFilename)
{
	//HACK: LightingShader initialises the matrix buffer with the wrong bytewidth (no shadow transform matrix)
	matrixBuffer->Release();

	// Set up matrix buffer
	D3D11_BUFFER_DESC matrixDesc;
	matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixDesc.MiscFlags = 0;
	matrixDesc.StructureByteStride = 0;
	matrixDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&matrixDesc, 0, &matrixBuffer);

	D3D11_SAMPLER_DESC sampDesc;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;

	// Everything outside of shadow map will be treated as NOT in shadow
	memset(&sampDesc.BorderColor, 1.f, sizeof(float) * 4);

	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MipLODBias = 0;

	renderer->CreateSamplerState(&sampDesc, &shadowMapSampler);
}
