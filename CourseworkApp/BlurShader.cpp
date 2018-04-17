#include "BlurShader.h"
#include "Utility.h"

BlurShader::BlurShader(ID3D11Device* device, int width, int height, HWND hwnd)
	:	ComputeShader(device, hwnd)
	,	mWidth(width)
	,	mHeight(height)
{
	// Create shaders
	ID3DBlob* hzShaderBlob = ShaderToBlob(L"blurhz_cs.cso", hwnd);
	device->CreateComputeShader(hzShaderBlob->GetBufferPointer(), hzShaderBlob->GetBufferSize(), NULL, &mHorizontalShader);

	ID3DBlob* vcShaderBlob = ShaderToBlob(L"blurvc_cs.cso", hwnd);
	device->CreateComputeShader(vcShaderBlob->GetBufferPointer(), vcShaderBlob->GetBufferSize(), NULL, &mVerticalShader);


	// Create texture
	D3D11_TEXTURE2D_DESC tempTexDesc;
	ZeroMemory(&tempTexDesc, sizeof(D3D11_TEXTURE2D_DESC));
	tempTexDesc.Width = mWidth;
	tempTexDesc.Height = mHeight;
	tempTexDesc.MipLevels = 1;
	tempTexDesc.ArraySize = 1;
	tempTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tempTexDesc.SampleDesc.Count = 1;
	tempTexDesc.Usage = D3D11_USAGE_DEFAULT;
	tempTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	tempTexDesc.CPUAccessFlags = 0;
	tempTexDesc.MiscFlags = 0;

	ID3D11Texture2D* tempTex = nullptr;
	device->CreateTexture2D(&tempTexDesc, NULL, &tempTex);

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	srvDesc.Format = tempTexDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = tempTexDesc.MipLevels;

	device->CreateShaderResourceView(tempTex, &srvDesc, &mIntermediateSRV);

	// Create unordered access view
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
	uavDesc.Format = tempTexDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	device->CreateUnorderedAccessView(tempTex, &uavDesc, &mIntermediateUAV);


	// Clean up
	tempTex->Release();
	hzShaderBlob->Release();
	vcShaderBlob->Release();
}


BlurShader::~BlurShader()
{
	mHorizontalShader->Release();
	mVerticalShader->Release();

	mIntermediateSRV->Release();
	mIntermediateUAV->Release();
}

void BlurShader::Execute(ID3D11DeviceContext * context, ID3D11ShaderResourceView * frameSRV, ID3D11UnorderedAccessView * frameUAV)
{
	const int numGroupsX = (int)ceilf(mWidth / (float)TG_SIZE);
	const int numGroupsY = (int)ceilf(mHeight / (float)TG_SIZE);

	// Horizontal pass
	context->CSSetShaderResources(0, 1, &frameSRV);
	context->CSSetUnorderedAccessViews(0, 1, &mIntermediateUAV, NULL);

	context->CSSetShader(mHorizontalShader, NULL, 0);
	context->Dispatch(numGroupsX, mHeight, 1);

	// NOTE: A resource cannot be bound to input and output at the same time
	UnsetCSShaderInputsAndOutputs(context);

	// Vertical pass
	context->CSSetShaderResources(0, 1, &mIntermediateSRV);
	context->CSSetUnorderedAccessViews(0, 1, &frameUAV, NULL);

	context->CSSetShader(mVerticalShader, NULL, 0);
	context->Dispatch(mWidth, numGroupsY, 1);

	// Cleanup
	UnsetCSShaderInputsAndOutputs(context);
	context->CSSetShader(NULL, NULL, 0);
}
