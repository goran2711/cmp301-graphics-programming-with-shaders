// Compute shader that blur an entire frame buffer with a two-pass gaussian blur

#pragma once
#include "ComputeShader.h"

class BlurShader : public ComputeShader
{
public:
	static constexpr uint32_t TG_SIZE = 256U;

	BlurShader(ID3D11Device* device, int width, int height, HWND hwnd);
	BlurShader(const BlurShader&) = delete;
	BlurShader& operator=(const BlurShader&) = delete;
	~BlurShader();

	void Execute(ID3D11DeviceContext* context, ID3D11ShaderResourceView* frameSRV, ID3D11UnorderedAccessView* frameUAV);

protected:
	int mWidth, mHeight;

	ID3D11ComputeShader* mHorizontalShader;
	ID3D11ComputeShader* mVerticalShader;

	ID3D11ShaderResourceView* mIntermediateSRV;
	ID3D11UnorderedAccessView* mIntermediateUAV;
};

