// Assorted functions to help reduce code duplication

#pragma once
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <string>

// Check if a number is a power of two
inline bool IsPow2(int num) { return (num > 0 && (num & (num - 1) == 0)); }

namespace
{
	//// Recursive variadic template for ensuring a list of types are all the same
	// Base case
	template <typename ... Ts>
	struct is_all_same : std::true_type {};

	// Recursive specialised template
	// is_all_same<T0, T1, Ts...>::value == true when expression 'std::is_same<T0, T1>::value && is_all_same<T0, Ts...>::value' == true
	template <typename T0, typename T1, typename ... Ts>
	struct is_all_same<T0, T1, Ts...> : std::integral_constant<bool,
		std::is_same<T0, T1>::value && is_all_same<T0, Ts...>::value>
	{
	};

	// Read compiled shader object into ID3DBlob
	ID3DBlob* ShaderToBlob(WCHAR* filename, HWND hwnd)
	{
		ID3DBlob* shaderBuffer = 0;

		// check file extension for correct loading function.
		std::wstring fn(filename);
		std::string::size_type idx;
		std::wstring extension;

		idx = fn.rfind('.');

		if (idx != std::string::npos)
		{
			extension = fn.substr(idx + 1);
		}
		else
		{
			// No extension found
			MessageBox(hwnd, L"Error finding vertex shader file", L"ERROR", MB_OK);
			exit(0);
		}

		// Load the texture in.
		if (extension != L"cso")
		{
			MessageBox(hwnd, L"Incorrect vertex shader file type", L"ERROR", MB_OK);
			exit(0);
		}

		// Reads compiled shader into buffer (bytecode).
		HRESULT result = D3DReadFileToBlob(filename, &shaderBuffer);
		if (result != S_OK)
		{
			MessageBox(NULL, filename, L"File ERROR", MB_OK);
			exit(0);
		}

		return shaderBuffer;
	}

	// Convenience variables and functions for resetting shader input and output
	static constexpr uint32_t MAX_NUM_SRV = 16U;
	static constexpr uint32_t MAX_NUM_UAV = 8U;

	static ID3D11ShaderResourceView* const RESET_SRV[MAX_NUM_SRV] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	static ID3D11UnorderedAccessView* const RESET_UAV[MAX_NUM_UAV] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	void UnsetPSShaderInputs(ID3D11DeviceContext* context)
	{
		context->PSSetShaderResources(0, MAX_NUM_SRV, RESET_SRV);
	}

	void UnsetCSShaderInputsAndOutputs(ID3D11DeviceContext* context)
	{
		context->CSSetShaderResources(0, MAX_NUM_SRV, RESET_SRV);
		context->CSSetUnorderedAccessViews(0, MAX_NUM_UAV, RESET_UAV, NULL);
	}
}
