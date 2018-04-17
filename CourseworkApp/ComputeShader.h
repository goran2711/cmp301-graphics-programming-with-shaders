// Base compute shader class. Pretty useless, really. Just stores a d3d11 device

#pragma once
#include <d3d11.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

class ComputeShader
{
public:
	ComputeShader(ID3D11Device* device, HWND hwnd);
	ComputeShader(const ComputeShader&) = delete;
	ComputeShader& operator=(const ComputeShader&) = delete;
	virtual ~ComputeShader() = default;

protected:
	ID3D11Device* mDevice = nullptr;
};

