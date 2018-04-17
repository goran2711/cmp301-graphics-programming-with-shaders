// Shader that performs basic ambient, diffuse and specular lighting--as well as fog and displacement mapping

#pragma once
#include "..\DXFramework\BaseShader.h"
#include "ShaderBuffers.h"

class Camera;

class LightingShader : public BaseShader
{
	static constexpr float DEFAULT_FOG_MIN = 512.f;
	static constexpr float DEFAULT_FOG_RANGE = 64.f;
	static const XMFLOAT4 DEFAULT_FOG_COLOUR;

public:
	static constexpr unsigned MAX_LIGHTS = 8; // corresponds to lighting_ps.hlsl

	using ShaderLight = BufferType::ShaderLight;
	using FogBufferType = BufferType::FogBufferType;
	using LightBufferType = BufferType::LightBufferType<MAX_LIGHTS>;
	using CameraBufferType = BufferType::CameraBufferType;
	using MaterialPropertiesType = BufferType::MaterialPropertiesType;

	enum LightType // corresponds to lighting_ps.hlsl
	{
		DIRECTIONAL_LIGHT,
		POINT_LIGHT,
		SPOT_LIGHT
	};

	LightingShader(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd, WCHAR* vs = L"lighting_vs.cso", WCHAR* ps = L"lighting_ps.cso");
	LightingShader(const LightingShader&) = delete;
	LightingShader& operator=(const LightingShader&) = delete;
	virtual ~LightingShader();

	// Add a new light to be submitted to the shader
	// Returns a handle which can be used to later change the properties of the light
	int addLight(LightType type);

	// Replaces the light as the given index with the one provided
	int copyLight(ShaderLight* light);

	// Copy another shader's properties (fog, light, etc.)
	void update(ID3D11DeviceContext* context, const LightingShader& other);

	// Functions for generating matrices necessary for shadow mapping
	XMMATRIX generateLightViewMatrix(int lightHandle) const;
	XMMATRIX generateOrthoShadowTransform(int lightHandle, float frustumDim, float n, float f) const;

	// Get a pointer to a light source with the handle returned by addLight
	ShaderLight* getLight(int idx);
	ShaderLight* getLights() { return lights[0].enabled ? &lights[0] : nullptr; }

	void disableLight(int idx) { lights[idx].enabled = false; }

	void setAmbient(float r, float g, float b, float a = 1.f) { mAmbient = { r, g, b, a }; }
	void setFogProperties(ID3D11DeviceContext* context, XMFLOAT4 fogColour, float fogMin, float fogRange);

	virtual void XM_CALLCONV setShaderParameters(ID3D11DeviceContext* context, FXMMATRIX world, CXMMATRIX view, CXMMATRIX projection, Camera* camera, ID3D11ShaderResourceView* texture, ID3D11ShaderResourceView* heightMap = nullptr);
	
	virtual void render(ID3D11DeviceContext* context, int vertexCount) override;

protected:
	// Constructor that does not call LightingShader::initShaders (for custom input layout--used by InstanceShader)
	LightingShader(ID3D11Device * device, ID3D11DeviceContext * context, HWND hwnd, WCHAR * ps, bool);

	virtual void initShader(WCHAR* vsFilename, WCHAR* psFilename) override;

	ID3D11Buffer* lightBuffer = nullptr;
	ID3D11Buffer* fogBuffer = nullptr;
	ID3D11Buffer* cameraBuffer = nullptr;
	ID3D11Buffer* materialBuffer = nullptr;

	int numLights = 0;
	ShaderLight lights[MAX_LIGHTS];

	XMFLOAT4 mAmbient;
	
	// Fog properties
	XMFLOAT4 mFogColour;
	float mFogMin = 0.f, mFogRange = 0.f;

private:
	// The "actual" initShader function. Made private so that it will not be accidentally used by derived classes,
	// such as the InstanceShader
	void initShader(WCHAR* vsFilename, WCHAR* psFilename, bool loadVS);
};

