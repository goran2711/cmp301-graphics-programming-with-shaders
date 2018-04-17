// Basic shader that does texture mapping

#pragma once
#include "..\DXFramework\BaseShader.h"

class TextureShader : public BaseShader
{
public:
	TextureShader(ID3D11Device* device, HWND hwnd);
	TextureShader(const TextureShader&) = delete;
	TextureShader& operator=(const TextureShader&) = delete;

	void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, ID3D11ShaderResourceView* texture);
	
	void render(ID3D11DeviceContext* context, int vertexCount) override;

protected:
	void initShader(WCHAR* vs, WCHAR* ps) override;
};

