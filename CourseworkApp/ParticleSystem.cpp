#include "ParticleSystem.h"
#include <random>
#include <fstream>
#include <D3Dcompiler.h>
#include "../DXFramework/Camera.h"
#include "Utility.h"

//// Particle system initialiation helpers
std::random_device gRandDevice;
std::default_random_engine eng(gRandDevice());

// Generate a random number between 0 and 1
float RandF()
{
	static std::uniform_real_distribution<float> dist;
	
	return dist(eng);
}

// Generate a random number between the provided range
float RandF(float a, float b)
{
	return a + RandF() * (b - a);
}

XMVECTOR RandUnitVec3()
{
	XMVECTOR one = XMVectorSet(1.f, 1.f, 1.f, 1.f);

	while (true)
	{
		XMVECTOR v = XMVectorSet(
			RandF(-1.f, 1.f),
			RandF(-1.f, 1.f),
			RandF(-1.f, 1.f),
			0.f
		);

		if (XMVector3Greater(v, one))
			continue;

		return XMVector3Normalize(v);
	}
}

ID3D11ShaderResourceView* CreateRandomTexture1DSRV(ID3D11Device* device)
{
	static constexpr unsigned int NUM_VALUES = 1024;

	XMFLOAT4 randomValues[NUM_VALUES];

	// Fill array with random values [-1, 1]
	for (int i = 0; i < NUM_VALUES; ++i)
	{
		randomValues[i].x = RandF(-1.f, 1.f);
		randomValues[i].y = RandF(-1.f, 1.f);
		randomValues[i].z = RandF(-1.f, 1.f);
		randomValues[i].w = RandF(-1.f, 1.f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = NUM_VALUES * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	// Create texture
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = NUM_VALUES;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ID3D11Texture1D* texture = nullptr;
	device->CreateTexture1D(&texDesc, &initData, &texture);

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	ID3D11ShaderResourceView* srv = nullptr;
	device->CreateShaderResourceView(texture, &viewDesc, &srv);

	texture->Release();
	return srv;
}

//// Particle system class
ParticleSystem::ParticleSystem(ID3D11Device * device, HWND hwnd)
{
	Init(device, hwnd);
}

ParticleSystem::~ParticleSystem()
{
	inputLayout->Release();

	vertexShaderUpdate->Release();
	geometryShaderUpdate->Release();

	vertexShaderDraw->Release();
	geometryShaderDraw->Release();
	pixelShader->Release();
	
	randomTexture->Release();

	initVertexBuffer->Release();
	drawVertexBuffer->Release();
	updateVertexBuffer->Release();

	perFrameBuffer->Release();
	fixedBuffer->Release();

	linearSampler->Release();

	depthStencilDisabledState->Release();
	depthWritesDisabledState->Release();
}

void XM_CALLCONV ParticleSystem::Draw(ID3D11DeviceContext * context, Camera * camera, FXMMATRIX projMatrix, float frameTime, float gameTime)
{
	// Presevere DSS
	UINT originalStencilRef = 1;
	ID3D11DepthStencilState* originalDSS;
	context->OMGetDepthStencilState(&originalDSS, &originalStencilRef);

	XMMATRIX viewProj = camera->getViewMatrix() * projMatrix;

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Update Per Frame Buffer
	context->Map(perFrameBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	PerFrameBufferType* perFrame = static_cast<PerFrameBufferType*>(mappedResource.pData);

	perFrame->eyePosW = camera->getPosition();
	perFrame->gameTime = gameTime;

	perFrame->emitPosW = emitPos;
	perFrame->frameTime = frameTime;
	
	perFrame->emitDirW = emitDir;
	perFrame->padding = 0.f;
	
	XMStoreFloat4x4(&perFrame->viewProj, viewProj);

	context->Unmap(perFrameBuffer, 0);

	// Set constant buffers, textures and sample states
	context->VSSetConstantBuffers(0, 1, &fixedBuffer);

	ID3D11Buffer* gsBuffers[2] = { fixedBuffer, perFrameBuffer };
	context->GSSetConstantBuffers(0, 2, gsBuffers);

	context->GSSetSamplers(0, 1, &linearSampler);
	context->GSSetShaderResources(0, 1, &randomTexture);

	// Prepare Input Assembly for the update pass
	context->IASetInputLayout(inputLayout);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	static constexpr UINT stride = sizeof(Particle);
	static constexpr UINT offset = 0;

	// Use the initialisation VB if this is the first frame the particle system is drawn
	context->IASetVertexBuffers(0, 1, 
								(firstRun ? &initVertexBuffer : &drawVertexBuffer),
								&stride, &offset);

	// Set Stream Output target
	ID3D11Buffer* pBuffer[1] = { updateVertexBuffer };
	context->SOSetTargets(1, pBuffer, &offset);

	// Set update stage shaders
	context->VSSetShader(vertexShaderUpdate, NULL, 0);
	context->GSSetShader(geometryShaderUpdate, NULL, 0);

	// Disable rasteriser stage (SO only)
	context->PSSetShader(NULL, NULL, 0);
	context->OMSetDepthStencilState(depthStencilDisabledState, 1);

	if (firstRun)
	{
		context->Draw(1, 0);
		firstRun = false;
	}
	else
		context->DrawAuto();

	// Unbind vertex buffer from Stream Output
	pBuffer[0] = nullptr;
	context->SOSetTargets(1, pBuffer, &offset);

	// Swap vertex buffers so that we render the one we streamed to
	std::swap(drawVertexBuffer, updateVertexBuffer);

	// Prepare the Input Assembly for the particle draw pass
	pBuffer[0] = drawVertexBuffer;
	context->IASetVertexBuffers(0, 1, pBuffer, &stride, &offset);

	// Set draw stage shaders
	context->VSSetShader(vertexShaderDraw, NULL, 0);
	context->GSSetShader(geometryShaderDraw, NULL, 0);
	context->PSSetShader(pixelShader, NULL, 0);

	// Disable depth writes when drawing particles
	context->OMSetDepthStencilState(depthWritesDisabledState, 1);

	context->DrawAuto();
	
	// Restore original DSS to re-enable depth writes
	context->OMSetDepthStencilState(originalDSS, originalStencilRef);
}

void ParticleSystem::Init(ID3D11Device* device, HWND hwnd)
{
	// Vertex Shader (Update)
	ID3DBlob* blobVertexUpdate = ShaderToBlob(L"particle_update_vs.cso", hwnd);
	device->CreateVertexShader(blobVertexUpdate->GetBufferPointer(), blobVertexUpdate->GetBufferSize(), 0, &vertexShaderUpdate);

	// Vertex Shader (Draw)
	ID3DBlob* blobVertexDraw = ShaderToBlob(L"particle_draw_vs.cso", hwnd);
	device->CreateVertexShader(blobVertexDraw->GetBufferPointer(), blobVertexDraw->GetBufferSize(), 0, &vertexShaderDraw);

	// Geometry Shader (Update)
	ID3DBlob* blobGeometryUpdate = ShaderToBlob(L"particle_update_gs.cso", hwnd);

	D3D11_SO_DECLARATION_ENTRY pDecl[] =
	{
		// stream number, semantic name, semantic index, start component, component count, output slot
		{ 0, "POSITION",	0, 0, 3, 0 },   
		{ 0, "VELOCITY",	0, 0, 3, 0 },     
		{ 0, "SIZE",		0, 0, 2, 0 },
		{ 0, "AGE",			0, 0, 1, 0 },
		{ 0, "TYPE",		0, 0, 1, 0 }
	};

	device->CreateGeometryShaderWithStreamOutput(blobGeometryUpdate->GetBufferPointer(), blobGeometryUpdate->GetBufferSize(), pDecl, sizeof(pDecl) / sizeof(pDecl[0]),
												 NULL, 0, D3D11_SO_NO_RASTERIZED_STREAM, NULL, &geometryShaderUpdate);

	// Geometry Shader (Draw)
	ID3DBlob* blobGeometryDraw = ShaderToBlob(L"particle_draw_gs.cso", hwnd);
	device->CreateGeometryShader(blobGeometryDraw->GetBufferPointer(), blobGeometryDraw->GetBufferSize(), 0, &geometryShaderDraw);

	// Fragment Shader (Draw)
	ID3DBlob* blobPixelDraw = ShaderToBlob(L"particle_draw_ps.cso", hwnd);
	device->CreatePixelShader(blobPixelDraw->GetBufferPointer(), blobPixelDraw->GetBufferSize(), 0, &pixelShader);

	// Input Layouts
	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,								D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",		0, DXGI_FORMAT_R32G32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE",		0, DXGI_FORMAT_R32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TYPE",		0, DXGI_FORMAT_R32_UINT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// NOTE: Both vertex shaders have the same input layout
	device->CreateInputLayout(inputLayoutDesc, sizeof(inputLayoutDesc) / sizeof(inputLayoutDesc[0]), blobVertexDraw->GetBufferPointer(), blobVertexDraw->GetBufferSize(), &inputLayout);

	//// Vertex Buffers
	// Init Vertex Buffer (Used to kick off particle system)
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));

	vertexBufferDesc.ByteWidth = sizeof(Particle);
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;

	Particle initParticle;
	ZeroMemory(&initParticle, sizeof(Particle));

	// These are the only two parameters that matter for emitters
	initParticle.age = 0.f;
	initParticle.type = EMITTER;

	D3D11_SUBRESOURCE_DATA initParticleData;
	initParticleData.pSysMem = &initParticle;
	initParticleData.SysMemPitch = sizeof(Particle);
	initParticleData.SysMemSlicePitch = 0;

	device->CreateBuffer(&vertexBufferDesc, &initParticleData, &initVertexBuffer);

	// Draw and Update Vertex Buffer
	vertexBufferDesc.ByteWidth = sizeof(Particle) * MAX_VERTICES;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	device->CreateBuffer(&vertexBufferDesc, 0, &drawVertexBuffer);
	device->CreateBuffer(&vertexBufferDesc, 0, &updateVertexBuffer);

	// Per Frame Constant Buffer
	D3D11_BUFFER_DESC perFrameDesc;
	ZeroMemory(&perFrameDesc, sizeof(D3D11_BUFFER_DESC));

	perFrameDesc.ByteWidth = sizeof(PerFrameBufferType);
	perFrameDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	perFrameDesc.Usage = D3D11_USAGE_DYNAMIC;
	perFrameDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	perFrameDesc.MiscFlags = 0;

	device->CreateBuffer(&perFrameDesc, 0, &perFrameBuffer);

	// Fixed Constant Buffer
	D3D11_BUFFER_DESC fixedDesc;
	ZeroMemory(&fixedDesc, sizeof(D3D11_BUFFER_DESC));

	fixedDesc.ByteWidth = sizeof(FixedBufferType);
	fixedDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	fixedDesc.Usage = D3D11_USAGE_IMMUTABLE;
	fixedDesc.CPUAccessFlags = 0;
	fixedDesc.MiscFlags = 0;

	static constexpr float acceleration[3] = { -1.f, -9.81f, 0.f };

	D3D11_SUBRESOURCE_DATA fixedData;
	fixedData.pSysMem = acceleration;
	fixedData.SysMemPitch = 0;
	fixedData.SysMemSlicePitch = 0;

	device->CreateBuffer(&fixedDesc, &fixedData, &fixedBuffer);

	// Random Texture Sampler
	D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.MipLODBias = 0;

	device->CreateSamplerState(&samplerDesc, &linearSampler);

	// Create 1D texture of random numbers for sampling in the shaders
	randomTexture = CreateRandomTexture1DSRV(device);

	// Set up depth stencil states
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
	depthStencilDesc.DepthEnable = true;

	// Disable depth/stencil writes (needed in conjunction with NULL fragment shader to disable rasteriser stage)
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = false;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateDepthStencilState(&depthStencilDesc, &depthWritesDisabledState);

	// Disable depth test and depth/stencil writes
	depthStencilDesc.DepthEnable = false;

	device->CreateDepthStencilState(&depthStencilDesc, &depthStencilDisabledState);
}
