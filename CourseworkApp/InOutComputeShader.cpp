#include "InOutComputeShader.h"

InOutComputeShader::InOutComputeShader(ID3D11Device* device, int width, int height, WCHAR* csoFilename, HWND hwnd)
	:	ComputeShader(device, hwnd)
	,	mWidth(width)
	,	mHeight(height)
{
	ID3DBlob* shaderBlob = ShaderToBlob(csoFilename, hwnd);
	device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &mComputeShader);

	shaderBlob->Release();
}

InOutComputeShader::~InOutComputeShader()
{
	mComputeShader->Release();
}

void InOutComputeShader::Execute(ID3D11DeviceContext * context)
{
	const int numGroupsX = (int)ceilf(mWidth / (float)TG_SIZE);

	context->CSSetShader(mComputeShader, NULL, 0);
	context->Dispatch(numGroupsX, mHeight, 1),

	// Cleanup
	UnsetCSShaderInputsAndOutputs(context);
	context->CSSetShader(NULL, NULL, 0);
}
