#include "BillboardingShader.h"
#include "..\DXFramework\Camera.h"

BillboardingShader::BillboardingShader(ID3D11Device * device, HWND hwnd)
	:	BaseShader(device, hwnd)
{
	initShader(NULL, NULL);
}

BillboardingShader::~BillboardingShader()
{
	mCameraBuffer->Release();
}

void BillboardingShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	context->PSSetSamplers(0, 1, &sampleState);
	BaseShader::render(context, vertexCount);

	// Unset geometry shader so it does not interfere
	context->GSSetShader(NULL, NULL, 0);
}

void XM_CALLCONV BillboardingShader::setShaderParameters(ID3D11DeviceContext * context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, ID3D11ShaderResourceView * texture, Camera * camera)
{
	D3D11_MAPPED_SUBRESOURCE map;

	// Map matrix buffer
	context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	MatrixBufferType* matrixData = static_cast<MatrixBufferType*>(map.pData);
	matrixData->world = world;
	matrixData->view = view;
	matrixData->projection = projection;

	context->Unmap(matrixBuffer, 0);

	// Map camera buffer
	context->Map(mCameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	CameraBufferType* cameraData = static_cast<CameraBufferType*>(map.pData);
	cameraData->position = camera->getPosition();

	context->Unmap(mCameraBuffer, 0);

	// Set constant buffers
	ID3D11Buffer* buffers[2] = { matrixBuffer, mCameraBuffer };
	context->GSSetConstantBuffers(0, 2, buffers);

	context->PSSetShaderResources(0, 1, &texture);
}

void BillboardingShader::initShader(WCHAR *, WCHAR *)
{
	loadVertexShader(L"passthrough_vs.cso");
	loadGeometryShader(L"billboard_gs.cso");
	loadPixelShader(L"texture_ps.cso");

	// Set up matrix buffer
	D3D11_BUFFER_DESC matrixDesc;
	matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixDesc.MiscFlags = 0;
	matrixDesc.StructureByteStride = 0;
	matrixDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&matrixDesc, 0, &matrixBuffer);

	// Set up camera buffer
	D3D11_BUFFER_DESC cameraDesc;
	cameraDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cameraDesc.ByteWidth = sizeof(CameraBufferType);
	cameraDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cameraDesc.MiscFlags = 0;
	cameraDesc.StructureByteStride = 0;
	cameraDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&cameraDesc, 0, &mCameraBuffer);
	
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
