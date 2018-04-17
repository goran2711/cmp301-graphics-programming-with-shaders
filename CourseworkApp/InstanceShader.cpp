#include "InstanceShader.h"
#include "Utility.h"
#include "../DXFramework/Camera.h"

InstanceShader::InstanceShader(ID3D11Device * device, ID3D11DeviceContext * context, HWND hwnd)
	:	LightingShader(device, context, hwnd, L"lighting_ps.cso", true)
{
	initShader(L"instance_vs.cso");
}

InstanceShader::~InstanceShader()
{
	instanceBuffer->Release();
}

void XM_CALLCONV InstanceShader::addInstance(FXMMATRIX world)
{
	assert(usedInstances + 1 <= MAX_INSTANCES);

	instances[usedInstances++].world = world;
}

void XM_CALLCONV InstanceShader::setShaderParameters(ID3D11DeviceContext * context, FXMMATRIX view, CXMMATRIX projection, Camera* camera, ID3D11ShaderResourceView * texture)
{
	D3D11_MAPPED_SUBRESOURCE map;
	context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	// Update matrix buffer
	MatrixBufferType* matrixPtr = static_cast<MatrixBufferType*>(map.pData);
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

	// Update instance buffer
	context->Map(instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

	InstanceBufferType* instancePtr = static_cast<InstanceBufferType*>(map.pData);
	memcpy_s(instancePtr, sizeof(InstanceBufferType) * MAX_INSTANCES, instances, sizeof(InstanceBufferType) * usedInstances);

	context->Unmap(instanceBuffer, 0);

	// Set constant buffer
	ID3D11Buffer* vsBuffers[2] = { matrixBuffer, cameraBuffer };
	context->VSSetConstantBuffers(0, 2, vsBuffers);

	ID3D11Buffer* psBuffers[2] = { lightBuffer, fogBuffer };
	context->PSSetConstantBuffers(0, 2, psBuffers);

	// Set textures
	context->PSSetShaderResources(0, 1, &texture);
}

void InstanceShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	UINT stride[1] = { sizeof(InstanceBufferType) };
	UINT offset[1] = { 0 };

	// VB slot 0 is set by BaseMesh::sendData
	// VS slot 1 contains instance data
	context->IASetVertexBuffers(1, 1, &instanceBuffer, stride, offset);
	context->IASetInputLayout(layout);

	context->PSSetSamplers(0, 1, &sampleState);

	context->VSSetShader(vertexShader, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);
	context->HSSetShader(hullShader, NULL, 0);
	context->DSSetShader(domainShader, NULL, 0);
	context->GSSetShader(geometryShader, NULL, 0);

	context->DrawIndexedInstanced(vertexCount, usedInstances, 0, 0, 0);

	// Reset instance count to prepare for next render call
	usedInstances = 0;
}

void InstanceShader::initShader(WCHAR * vs)
{
	loadVertexShader(vs);

	// HACK: Lighting shader initialises the matrix buffer with invalid bytewidth (LightingShader::MatrixBufferType != InstanceShader::MatrixBufferType)
	matrixBuffer->Release();

	// Set up matrix buffer with correct bytewidth
	D3D11_BUFFER_DESC matrixDesc;
	matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixDesc.MiscFlags = 0;
	matrixDesc.StructureByteStride = 0;
	matrixDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&matrixDesc, 0, &matrixBuffer);

	// Create vertex buffer containing instance data
	D3D11_BUFFER_DESC instanceDesc;
	ZeroMemory(&instanceDesc, sizeof(D3D11_BUFFER_DESC));
	instanceDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	instanceDesc.ByteWidth = sizeof(InstanceBufferType) * MAX_INSTANCES;
	instanceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	instanceDesc.Usage = D3D11_USAGE_DYNAMIC;

	renderer->CreateBuffer(&instanceDesc, 0, &instanceBuffer);
}

void InstanceShader::loadVertexShader(WCHAR * vs)
{
	ID3DBlob* bytecode = ShaderToBlob(vs, hwnd);
	renderer->CreateVertexShader(bytecode->GetBufferPointer(), bytecode->GetBufferSize(), NULL, &vertexShader);

	// Create input layout that also takes per-instance data
	D3D11_INPUT_ELEMENT_DESC inputDesc[7] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,		0,							  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",	  0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "WORLD",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "WORLD",    3, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

	renderer->CreateInputLayout(inputDesc, sizeof(inputDesc) / sizeof(inputDesc[0]), bytecode->GetBufferPointer(), bytecode->GetBufferSize(), &layout);

	bytecode->Release();
}
