#include "LightingShader.h"
#include "..\DXFramework\Light.h"
#include "..\DXFramework\Camera.h"

/* static */ const XMFLOAT4 LightingShader::DEFAULT_FOG_COLOUR = { 0.39f, 0.58f, 0.92f, 1.0f };

LightingShader::LightingShader(ID3D11Device * device, ID3D11DeviceContext * context, HWND hwnd, WCHAR * vs, WCHAR * ps)
	:	BaseShader(device, hwnd)
{
	ZeroMemory(lights, sizeof(ShaderLight) * MAX_LIGHTS);

	initShader(vs, ps, true);
	setFogProperties(context, DEFAULT_FOG_COLOUR, DEFAULT_FOG_MIN, DEFAULT_FOG_RANGE);
}

// Constructor used by InstanceShader--does not load the vertex shader, as the InstanceShader wants to create its own Input Layout
LightingShader::LightingShader(ID3D11Device * device, ID3D11DeviceContext * context, HWND hwnd, WCHAR * ps, bool)
	:	BaseShader(device, hwnd)
{
	ZeroMemory(lights, sizeof(ShaderLight) * MAX_LIGHTS);

	initShader(L"", ps, false);
	setFogProperties(context, DEFAULT_FOG_COLOUR, DEFAULT_FOG_MIN, DEFAULT_FOG_RANGE);
}

LightingShader::~LightingShader()
{
	lightBuffer->Release();
	fogBuffer->Release();
	cameraBuffer->Release();
	materialBuffer->Release();
}

int LightingShader::addLight(LightType type)
{
	if (numLights + 1 >= MAX_LIGHTS)
		return -1;

	int idx = numLights++;
	lights[idx].type = type;
	lights[idx].enabled = true;

	return idx;
}

int LightingShader::copyLight(ShaderLight * light)
{
	int idx = addLight(static_cast<LightType>(light->type));

	if (idx < 0)
		return idx;

	lights[idx] = *light;
	return idx;
}

void LightingShader::update(ID3D11DeviceContext* context, const LightingShader & other)
{
	mAmbient = other.mAmbient;

	// NOTE: Calling this function every frame is rather wasteful
	setFogProperties(context, other.mFogColour, other.mFogMin, other.mFogRange);
	
	numLights = other.numLights;
	memcpy_s(lights, sizeof(ShaderLight) * MAX_LIGHTS, other.lights, sizeof(ShaderLight) * MAX_LIGHTS);
}

XMMATRIX LightingShader::generateLightViewMatrix(int lightHandle) const
{
	auto light = lights[lightHandle];

	XMVECTOR position = XMLoadFloat4(&light.position);
	XMVECTOR direction = XMLoadFloat4(&light.direction);

	XMMATRIX viewMatrix = XMMatrixLookAtLH(position, position + direction, XMVectorSet(0.f, 1.f, 0.f, 0.f));
	return viewMatrix;
}

XMMATRIX LightingShader::generateOrthoShadowTransform(int lightHandle, float frustumDim, float n, float f) const
{
	// Prepare shadow transform
	XMMATRIX lightView = generateLightViewMatrix(lightHandle);
	XMMATRIX lightProjection = XMMatrixOrthographicLH(frustumDim, frustumDim, n, f);

	// Matrix for transforming homogenous clip space to UV space
	static const XMMATRIX toTextureSpace(
		0.5f, 0.f, 0.f, 0.f,
		0.f, -0.5f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.5f, 0.5f, 0.f, 1.f
	);

	return lightView * lightProjection * toTextureSpace;
}

auto LightingShader::getLight(int idx) -> ShaderLight*
{
	if (idx >= MAX_LIGHTS)
		return nullptr;

	return lights[idx].enabled ? &lights[idx] : nullptr;
}

void LightingShader::setFogProperties(ID3D11DeviceContext* context, XMFLOAT4 fogColour, float fogMin, float fogRange)
{
	D3D11_MAPPED_SUBRESOURCE mappedRes;
	context->Map(fogBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);

	FogBufferType* fog = static_cast<FogBufferType*>(mappedRes.pData);
	ZeroMemory(fog, sizeof(FogBufferType));

	fog->fogColour = fogColour;
	fog->fogMin = fogMin;
	fog->fogRange = fogRange;

	context->Unmap(fogBuffer, 0);

	mFogColour = fogColour;
	mFogMin = fogMin;
	mFogRange = fogRange;
}

void XM_CALLCONV LightingShader::setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* heightMap)
{
	D3D11_MAPPED_SUBRESOURCE map;
	context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	// Update matrix buffer
	MatrixBufferType* matrixPtr = static_cast<MatrixBufferType*>(map.pData);
	matrixPtr->world = world;
	matrixPtr->view = view;
	matrixPtr->projection = projection;

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

	// Update material properties
	context->Map(materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	MaterialPropertiesType* materialPtr = static_cast<MaterialPropertiesType*>(map.pData);
	materialPtr->gUseHeightMap = (heightMap != nullptr);

	context->Unmap(materialBuffer, 0);

	// 'Dispatch' constant buffer
	ID3D11Buffer* vsBuffers[3] = { matrixBuffer, cameraBuffer, materialBuffer };
	context->VSSetConstantBuffers(0, 3, vsBuffers);

	ID3D11Buffer* psBuffers[3] = { lightBuffer, fogBuffer, materialBuffer };
	context->PSSetConstantBuffers(0, 3, psBuffers);

	// Send textures
	context->VSSetShaderResources(0, 1, &heightMap);
	context->PSSetShaderResources(0, 1, &texture);
}

void LightingShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	context->VSSetSamplers(0, 1, &sampleState);
	context->PSSetSamplers(0, 1, &sampleState);

	BaseShader::render(context, vertexCount);
}

void LightingShader::initShader(WCHAR * vsFilename, WCHAR * psFilename)
{
	initShader(vsFilename, psFilename, true);
}

void LightingShader::initShader(WCHAR * vsFilename, WCHAR * psFilename, bool loadVS)
{
	// Create shaders and input layouts
	if (loadVS)
	{
		loadVertexShader(vsFilename);
	}

	loadPixelShader(psFilename);

	// Set up matrix buffer
	D3D11_BUFFER_DESC matrixDesc;
	matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixDesc.MiscFlags = 0;
	matrixDesc.StructureByteStride = 0;
	matrixDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&matrixDesc, 0, &matrixBuffer);

	// Set up lighting buffer
	D3D11_BUFFER_DESC lightDesc;
	lightDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	lightDesc.ByteWidth = sizeof(LightBufferType);
	lightDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	lightDesc.MiscFlags = 0;
	lightDesc.StructureByteStride = 0;
	lightDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&lightDesc, 0, &lightBuffer);

	// Set up fog buffer (only difference from the others is the size...)
	D3D11_BUFFER_DESC fogDesc;
	fogDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fogDesc.ByteWidth = sizeof(FogBufferType);
	fogDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	fogDesc.MiscFlags = 0;
	fogDesc.StructureByteStride = 0;
	fogDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&fogDesc, 0, &fogBuffer);

	// Set up camera buffer
	D3D11_BUFFER_DESC cameraDesc;
	cameraDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraDesc.ByteWidth = sizeof(CameraBufferType);
	cameraDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraDesc.MiscFlags = 0;
	cameraDesc.StructureByteStride = 0;
	cameraDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&cameraDesc, 0, &cameraBuffer);

	// Set up material buffer
	D3D11_BUFFER_DESC materialDes;
	materialDes.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	materialDes.ByteWidth = sizeof(MaterialPropertiesType);
	materialDes.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	materialDes.MiscFlags = 0;
	materialDes.StructureByteStride = 0;
	materialDes.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&materialDes, 0, &materialBuffer);

	// Set up sampler
	D3D11_SAMPLER_DESC sampDesc;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MipLODBias = 0;

	renderer->CreateSamplerState(&sampDesc, &sampleState);
}
