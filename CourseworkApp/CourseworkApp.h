// Application.h
#ifndef _APP1_H
#define _APP1_H

//// STL Includes
#include <memory>

//// Framework Includes
#include "../DXFramework/DXF.h"

//// Project Includes
// Shaders
#include "ColourShader.h"
#include "LightingShader.h"
#include "WaveShader.h"
#include "BillboardingShader.h"
#include "LightingShadowShader.h"
#include "BlurShader.h"
#include "InOutComputeShader.h"
#include "TextureShader.h"
#include "InstanceShader.h"

// Meshes
#include "MeshInstance.h"
#include "TessellatedPlane.h"

// Misc.
#include "ParticleSystem.h"
#include "BoundingVolume.h"

#define CLEAR_COLOUR	0.39f, 0.58f, 0.92f, 1.0f

class CourseworkApp : public BaseApplication
{
	template <typename T>
	using Pointer = std::unique_ptr<T>;
	
public:
	static constexpr int SHADOW_MAP_RESOLUTION = 2048;
	static constexpr float LIGHT_PROJECTION_FRUSTUM_DIM = 256.f;

	static constexpr int NUM_MODELS_X = 6;
	static constexpr int NUM_MODELS_Y = 6;
	static constexpr int NUM_MODELS_Z = 6;
	static constexpr int TOTAL_MODELS = NUM_MODELS_X * NUM_MODELS_Y * NUM_MODELS_Z;

	void init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input* in) override;

	bool frame() override;

protected:
	bool render() override;
	void gui();

	void XM_CALLCONV renderScene(FXMMATRIX viewMatrix, CXMMATRIX projectionMatrix, bool isShadowPass);
	void postProcessing();
	void XM_CALLCONV showRenderTexture(ID3D11ShaderResourceView* texture);

	// Initialise functions
	void initialiseInformation();
	void initialiseShaders(int screenWidth, int screenHeight, HWND hwnd);
	void initialiseRenderTextures(int screenWidth, int screenHeight);
	void initialiseTextures();
	void initialiseMeshes(int screenWidth, int screenHeight);
	void initialiseMeshInstances();
	void initialiseCullableMeshInstances();
	void initialiseLights();
	void initialiseCullingMatrix(int screenWidth, int screenHeight);
	void initialiseBoundingVolume();

	// Update function
	void updateDirectionalLight();
	void updatePointLights();
	void updateSpotlight();
	void synchroniseLights();

	void updateCullingMatrix();

	void createShadowMap(const LightingShader::ShaderLight& light);

private:
	// Shaders
	Pointer<ColourShader> mColourShader;
	// This is the "master" lighting shader, i.e. the one whose lights are manipulated and copied over to other shaders using lights
	Pointer<LightingShader> mLightingShader;
	Pointer<WaveShader> mWaveShader;
	Pointer<BillboardingShader> mBillboardingShader;
	Pointer<LightingShadowShader> mLightingShadowShader;
	Pointer<BlurShader> mBlurShader;
	Pointer<InOutComputeShader> mCoCShader;
	Pointer<InOutComputeShader> mMergeBuffersShader;
	Pointer<TextureShader> mTextureShader;
	Pointer<InstanceShader> mInstanceShader;

	// Meshes
	MeshInstance mCubeMesh;
	MeshInstance mTessellatedPlaneMesh;
	MeshInstance mBillboardPoint;
	MeshInstance mCullableMeshes[TOTAL_MODELS];

	// Render Textures
	Pointer<RenderTexture> mShadowMap;
	Pointer<RenderTexture> mRenderTex;
	Pointer<RenderTexture> mBlurredRenderTex;
	Pointer<RenderTexture> mCoCMap;

	// Misc.
	MeshInstance mOrthoMesh;
	Pointer<ParticleSystem> mParticleSystem;
	std::string mInformation;
	float mTotalTime = 0.f;

	ID3D11ShaderResourceView* mDebugRenderTexture = nullptr;

	// Light handles
	int mDirectionalLight = -1;
	int mPointLightA = -1;
	int mPointLightB = -1;
	int mSpotlight = -1;

	// Shadow settings
	bool mDoShadows = true;

	// Fog settings
	XMFLOAT4 mFogColour = { CLEAR_COLOUR };
	float mFogMin = 32.f;
	float mFogRange = 128.f;

	// Tessellation settings
	int mMinTess = WaveShader::DEFAULT_MIN_TESS;
	int mMaxTess = WaveShader::DEFAULT_MAX_TESS;
	float mMinTessDistance = WaveShader::DEFAULT_MIN_TESS_DISTANCE;
	float mMaxTessDistance = WaveShader::DEFAULT_MAX_TESS_DISTANCE;

	// Post processing settings
	bool mDoBlur = false;
	bool mDoDoF = false;

	// Culling
	bool mDoCulling = true;
	bool mUseInstancing = true;

	float mCullingFov = XM_PIDIV2;
	float mAspectRatio;
	XMFLOAT4X4 mCullingMatrix;

	int mRenderedModels = 0;
	Pointer<BoundingVolume> mBoundingVolume;

	// Pointer to the mesh contained within the bounding volume that will be drawn with the InstanceShader
	BaseMesh* mMeshToInstance = nullptr;
};

#endif
