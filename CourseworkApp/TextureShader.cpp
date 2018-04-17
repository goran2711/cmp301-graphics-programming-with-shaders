#include "TextureShader.h"

TextureShader::TextureShader(ID3D11Device * device, HWND hwnd)
	:	BaseShader(device, hwnd)
{
	initShader(L"texture_vs.cso", L"texture_ps.cso");
}

void XM_CALLCONV TextureShader::setShaderParameters(ID3D11DeviceContext * context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, ID3D11ShaderResourceView * texture)
{
	D3D11_MAPPED_SUBRESOURCE mapped;

	context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	MatrixBufferType* matrixBuf = static_cast<MatrixBufferType*>(mapped.pData);
	matrixBuf->world = world;
	matrixBuf->view = view;
	matrixBuf->projection = projection;

	context->Unmap(matrixBuffer, 0);

	context->VSSetConstantBuffers(0, 1, &matrixBuffer);

	context->PSSetShaderResources(0, 1, &texture);
	context->PSSetSamplers(0, 1, &sampleState);
}

void TextureShader::render(ID3D11DeviceContext * context, int vertexCount)
{
	context->PSSetSamplers(0, 1, &sampleState);

	BaseShader::render(context, vertexCount);
}

void TextureShader::initShader(WCHAR * vs, WCHAR * ps)
{
	loadVertexShader(vs);
	loadPixelShader(ps);

	D3D11_BUFFER_DESC matrixDesc;
	ZeroMemory(&matrixDesc, sizeof(D3D11_BUFFER_DESC));
	matrixDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	renderer->CreateBuffer(&matrixDesc, 0, &matrixBuffer);

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	renderer->CreateSamplerState(&samplerDesc, &sampleState);
}
