// Generic compute shader that takes some inputs and produces one output without the use of intermediate buffers.

#pragma once
#include "ComputeShader.h"
#include "Utility.h"

class InOutComputeShader : public ComputeShader
{
public:
	static constexpr int TG_SIZE = 256;

	InOutComputeShader(ID3D11Device* device, int width, int height, WCHAR* csoFilename, HWND hwnd);
	InOutComputeShader(const InOutComputeShader&) = delete;
	InOutComputeShader& operator=(const InOutComputeShader&) = delete;
	~InOutComputeShader();

	// Template that can execute a compute shader with between 1 and 16 shader resource views and one unordered access view
	template <typename ... ResourceView>
	void Execute(ID3D11DeviceContext* context, ID3D11UnorderedAccessView* output, ResourceView ... inputs)
	{
		constexpr size_t numInputs = sizeof...(inputs);

		// Fail to compile if trying to bind too many inputs
		static_assert(numInputs < MAX_NUM_SRV, "Cannot bind more than 16 shader resource views");

		// Fail to compile if trying to provide anything but shader resource view
		static_assert(is_all_same<ID3D11ShaderResourceView*, ResourceView...>(), "Compute shader inputs must be ID3D11ShaderResourceView");

		// Fail to compile if sizeof...(inputs) < 1
		ID3D11ShaderResourceView* inputs[] = { inputs... };

		context->CSSetShaderResources(0, numInputs, inputs);
		context->CSSetUnorderedAccessViews(0, 1, &output, 0);

		Execute(context);
	}

private:
	void Execute(ID3D11DeviceContext* context);

	int mWidth, mHeight;
	ID3D11ComputeShader* mComputeShader;
};

