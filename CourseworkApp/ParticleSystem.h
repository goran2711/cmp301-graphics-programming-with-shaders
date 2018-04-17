// Particle system utilising the geometry and stream-out stage to do everything on the GPU

#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>

using namespace DirectX;

class Camera;

class ParticleSystem
{
	enum ParticleType
	{
		EMITTER = 0,
		FLARE = 1
	};

	struct Particle
	{
		XMFLOAT3 initialPositionW;
		XMFLOAT3 initialVeclocityW;

		// UNUSED
		XMFLOAT2 sizeW;

		float age;
		uint32_t type;
	};

	struct PerFrameBufferType
	{
		XMFLOAT3 eyePosW;
		float gameTime;
		
		XMFLOAT3 emitPosW;

		// UNUSED
		float frameTime;

		XMFLOAT3 emitDirW;
		float padding;

		XMFLOAT4X4 viewProj;
	};

	struct FixedBufferType
	{
		XMFLOAT3 accelW;
		float padding;
	};

public:
	static constexpr UINT MAX_VERTICES = 10'000;

	ParticleSystem(ID3D11Device* device, HWND hwnd);
	ParticleSystem(const ParticleSystem&) = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;
	~ParticleSystem();

	void SetEmitPos(const XMFLOAT3& newEmitPos) { emitPos = newEmitPos; }
	void SetEmitDir(const XMFLOAT3& newEmitDir) { emitDir = newEmitDir; }

	void XM_CALLCONV Draw(ID3D11DeviceContext* context, Camera* camera, FXMMATRIX projMatrix, float frameTime, float gameTime);

private:
	void Init(ID3D11Device* device, HWND hwnd);

	ID3D11InputLayout* inputLayout = nullptr;

	ID3D11VertexShader* vertexShaderUpdate = nullptr;
	ID3D11GeometryShader* geometryShaderUpdate = nullptr;

	ID3D11VertexShader* vertexShaderDraw = nullptr;
	ID3D11GeometryShader* geometryShaderDraw = nullptr;

	ID3D11PixelShader* pixelShader = nullptr;

	ID3D11ShaderResourceView* randomTexture = nullptr;

	ID3D11Buffer* initVertexBuffer = nullptr;
	ID3D11Buffer* drawVertexBuffer = nullptr;
	ID3D11Buffer* updateVertexBuffer = nullptr;

	ID3D11Buffer* perFrameBuffer = nullptr;
	ID3D11Buffer* fixedBuffer = nullptr;

	ID3D11SamplerState* linearSampler = nullptr;

	// NOTE: These would be better kept in the D3D class or some other place with all the other DSSes
	ID3D11DepthStencilState* depthStencilDisabledState = nullptr;
	ID3D11DepthStencilState* depthWritesDisabledState = nullptr;

	bool firstRun = true;

	XMFLOAT3 emitPos;
	XMFLOAT3 emitDir;
};

