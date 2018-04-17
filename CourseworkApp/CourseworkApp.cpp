#include "CourseworkApp.h"
#include "MeshManager.h"
#include "Utility.h"
#include <sstream>

void CourseworkApp::init(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight, Input *in)
{
	// Call super/parent init function (required!)
	BaseApplication::init(hinstance, hwnd, screenWidth, screenHeight, in);

	// Create the string for the information tooltip
	initialiseInformation();

	// Create shaders
	initialiseShaders(screenWidth, screenHeight, hwnd);

	// Create Render Textures
	initialiseRenderTextures(screenWidth, screenHeight);

	// Load textures
	initialiseTextures();

	// Load meshes
	initialiseMeshes(screenWidth, screenHeight);

	// Assign meshes and initialise mesh instances
	initialiseMeshInstances();

	// Create and initialise bounding volume
	initialiseBoundingVolume();

	// Initialise lighting
	initialiseLights();

	// Create particle system
	mParticleSystem = std::make_unique<ParticleSystem>(renderer->getDevice(), hwnd);

	// Initialise particle system
	mParticleSystem->SetEmitPos({ 0.f, 0.f, 0.f });

	// Initialise culling settings
	initialiseCullingMatrix(screenWidth, screenHeight);
}

bool CourseworkApp::frame()
{
	if (!BaseApplication::frame())
		return false;

	mTotalTime += timer->getFrameTime();

	//// Update logic
	camera->update();

	// Have particle system follow the camera
	mParticleSystem->SetEmitPos(camera->getPosition());

	// Rotate cube
	static float angle = 0.f;
	angle += timer->getFrameTime();
	angle = fmodf(angle, XM_2PI);

	mCubeMesh.SetRotation(XMQuaternionRotationNormal(XMVectorSet(0.f, 1.f, 0.f, 0.f), sinf(angle)));

	// Move light around
	updateDirectionalLight();
	updatePointLights();
	updateSpotlight();
	synchroniseLights();

	// Render the graphics.
	if (!render())
		return false;

	return true;
}

bool CourseworkApp::render()
{
	//// Clear the screen
	renderer->beginScene(CLEAR_COLOUR);

	// Generate shadow map
	if (mDoShadows)
	{
		auto directionalLight = mLightingShadowShader->getLight(mDirectionalLight);
		createShadowMap(*directionalLight);
	}

	//// Render the scene
	XMMATRIX viewMatrix = camera->getViewMatrix();
	XMMATRIX projectionMatrix = renderer->getProjectionMatrix();

	if (mDoDoF)
	{
		// Render to texture when doing DoF, because I need back buffer as output when merging
		mRenderTex->setRenderTarget(renderer->getDeviceContext());
		mRenderTex->clearRenderTarget(renderer->getDeviceContext(), CLEAR_COLOUR);
	}

	renderScene(viewMatrix, projectionMatrix, false);

	if (mDoBlur || mDoDoF)
		postProcessing();

	// Display render texture
	if (mDebugRenderTexture)
		showRenderTexture(mDebugRenderTexture);

	// Render GUI
	gui();

	//// Present the rendered scene to the screen.
	renderer->endScene();

	return true;
}

void CourseworkApp::gui()
{
	// Force turn off on Geometry shader
	renderer->getDeviceContext()->GSSetShader(NULL, NULL, 0);

	// Information
	ImGui::TextDisabled("Controls");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("WASD: Move\nQ: Move down\nE: Move up\nT: Toggle wireframe\nSPACE: Capture/release cursor\nSHIFT: Move fast");

	ImGui::SameLine();
	ImGui::TextDisabled("             What am I looking at?");
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip(mInformation.c_str());

	// Debug info
	ImGui::NewLine();
	ImGui::Text("FPS: %.0f", timer->getFPS());
	ImGui::Text("Frame time: %.2f ms", timer->getFrameTime());
	ImGui::Text("Visible models: %d / %d", mRenderedModels, TOTAL_MODELS);

	// Fog settings
	ImGui::NewLine();
	if (ImGui::CollapsingHeader("Fog"))
	{
		bool changedFog = ImGui::ColorEdit4("Fog colour", reinterpret_cast<float*>(&mFogColour));
		changedFog |= ImGui::SliderFloat("Fog min", &mFogMin, 1.f, 64.f);
		changedFog |= ImGui::SliderFloat("Fog range", &mFogRange, 1.f, 128.f);

		if (changedFog)
		{
			mLightingShader->setFogProperties(renderer->getDeviceContext(), mFogColour, mFogMin, mFogRange);
			mLightingShadowShader->update(renderer->getDeviceContext(), *mLightingShader);
			mWaveShader->update(renderer->getDeviceContext(), *mLightingShader);
		}
	}

	// Tessellation settings
	if (ImGui::CollapsingHeader("Tessellation"))
	{
		bool changedTess = ImGui::SliderInt("Min factor", &mMinTess, 0, mMaxTess);
		changedTess |= ImGui::SliderInt("Max factor", &mMaxTess, mMinTess, 6);
		changedTess |= ImGui::SliderFloat("Min distance", &mMinTessDistance, mMaxTessDistance, 128.f);
		changedTess |= ImGui::SliderFloat("Max distance", &mMaxTessDistance, 1.f, mMinTessDistance);

		if (changedTess)
			mWaveShader->setTessellationProperties(renderer->getDeviceContext(), mMinTess, mMaxTess, mMinTessDistance, mMaxTessDistance);
	}

	// Post processing settings
	if (ImGui::CollapsingHeader("Post processing"))
	{
		if (ImGui::Checkbox("Blur", &mDoBlur))
			mDoDoF = false;
		else if (ImGui::Checkbox("DoF", &mDoDoF))
			mDoBlur = false;
	}

	// Culling settings
	if (ImGui::CollapsingHeader("Frustum culling"))
	{
		if (ImGui::SliderFloat("Fov", &mCullingFov, XM_1DIV2PI, XM_PIDIV2))
			updateCullingMatrix();

		ImGui::Checkbox("Culling", &mDoCulling);
		ImGui::Checkbox("Hardware instancing", &mUseInstancing);
	}

	// Debug render textures
	if (ImGui::CollapsingHeader("Render textures"))
	{
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Shadow map: active when shadows mapping is on\nCircle of confusion and (blurred) frame: active when DoF is on\n\nNot all render textures clear themselves after the technique has been turned off");

		static int choice = 0;
		ImGui::RadioButton("Shadow map", &choice, 0);
		ImGui::RadioButton("Circle of confusion", &choice, 1);
		ImGui::RadioButton("Frame", &choice, 2);
		ImGui::RadioButton("Blurred frame", &choice, 3);

		switch (choice)
		{
			case 0:
				mDebugRenderTexture = mShadowMap->getDepthShaderResourceView();
				break;
			case 1:
				mDebugRenderTexture = mCoCMap->getShaderResourceView();
				break;
			case 2:
				mDebugRenderTexture = mRenderTex->getShaderResourceView();
				break;
			case 3:
				mDebugRenderTexture = mBlurredRenderTex->getShaderResourceView();
				break;
		}
	}
	else
		mDebugRenderTexture = nullptr;

	// Shadow settings
	ImGui::NewLine();
	if (ImGui::Checkbox("Shadows", &mDoShadows))
	{
		// Disabling shadows only stops rendering to the shadow map
		// Clearing it keeps the shadows of the last frame with shadows activated from lingering
		mShadowMap->clearRenderTarget(renderer->getDeviceContext(), CLEAR_COLOUR);
	}

	// Render UI
	ImGui::Render();
}

void XM_CALLCONV CourseworkApp::renderScene(FXMMATRIX viewMatrix, CXMMATRIX projectionMatrix, bool isShadowPass)
{
	XMMATRIX worldMatrix = XMMatrixIdentity();

	// Draw a cube
	worldMatrix = mCubeMesh.GetWorldMatrix();
	mLightingShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, camera, textureMgr->getTexture("cliff_d"), textureMgr->getTexture("cliff_h"));
	mCubeMesh.Draw(renderer->getDeviceContext(), mLightingShader);

	// Render cullable meshes
	if (mDoCulling)
	{
		// Projection matrix that is used for culling
		XMMATRIX cullingMatrix = XMLoadFloat4x4(&mCullingMatrix);

		BoundingFrustum cameraFrustum;
		BoundingFrustum::CreateFromMatrix(cameraFrustum, cullingMatrix);

		// Transform frustum to view space
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(viewMatrix), viewMatrix);
		cameraFrustum.Transform(cameraFrustum, invView);

		if (mUseInstancing)
		{
			mRenderedModels = mBoundingVolume->GetVisibleGeometry(cameraFrustum, *mInstanceShader);

			mMeshToInstance->sendData(renderer->getDeviceContext());
			mInstanceShader->setShaderParameters(renderer->getDeviceContext(), viewMatrix, projectionMatrix, camera, textureMgr->getTexture("bricks"));
			mInstanceShader->render(renderer->getDeviceContext(), mMeshToInstance->getIndexCount());
		}
		else
		{
			std::vector<MeshInstance*> visibleInstances;
			visibleInstances.reserve(TOTAL_MODELS);

			mBoundingVolume->GetVisibleGeometry(cameraFrustum, visibleInstances);

			for (auto& mesh : visibleInstances)
			{
				worldMatrix = mesh->GetWorldMatrix();
				mLightingShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, camera, textureMgr->getTexture("bricks"));
				mesh->Draw(renderer->getDeviceContext(), mLightingShader);
			}
		}
	}
	else
	{
		mRenderedModels = TOTAL_MODELS;

		for (auto& mesh : mCullableMeshes)
		{
			worldMatrix = mesh.GetWorldMatrix();
			mLightingShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, camera, textureMgr->getTexture("bricks"));
			mesh.Draw(renderer->getDeviceContext(), mLightingShader);
		}
	}

	// This stuff should not cast shadows
	if (!isShadowPass)
	{
		// Draw a billboarded sprite
		worldMatrix = mBillboardPoint.GetWorldMatrix();
		mBillboardingShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, textureMgr->getTexture("bunny"), camera);
		mBillboardPoint.Draw(renderer->getDeviceContext(), mBillboardingShader);

		// Draw a tessellated plane with shadow cast on it
		XMMATRIX shadowTransform = mLightingShader->generateOrthoShadowTransform(mDirectionalLight, LIGHT_PROJECTION_FRUSTUM_DIM, SCREEN_NEAR, SCREEN_DEPTH);

		worldMatrix = mTessellatedPlaneMesh.GetWorldMatrix();
		mWaveShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, shadowTransform, camera, textureMgr->getTexture("default"), mShadowMap->getDepthShaderResourceView(), mTotalTime);
		mTessellatedPlaneMesh.Draw(renderer->getDeviceContext(), mWaveShader);

		// Render the particle system
		mParticleSystem->Draw(renderer->getDeviceContext(), camera, projectionMatrix, timer->getFrameTime(), mTotalTime);
	}
}

void CourseworkApp::postProcessing()
{
	// Unbind back buffer as render target (mShadowMap chosen arbitrarily--as long as it's not the back buffer)
	mShadowMap->setRenderTarget(renderer->getDeviceContext());

	if (mDoBlur)
		// Execute blur
		mBlurShader->Execute(renderer->getDeviceContext(), renderer->getShaderResourceView(), renderer->getUnorderedAccessView());
	else if (mDoDoF)
	{
		// Create CoC map from depth map
		mCoCShader->Execute(renderer->getDeviceContext(), mCoCMap->getUnorderedAccessView(), mRenderTex->getDepthShaderResourceView());

		// Execute blur, storing the blurred result in a render texture
		mBlurShader->Execute(renderer->getDeviceContext(), mRenderTex->getShaderResourceView(), mBlurredRenderTex->getUnorderedAccessView());

		// Merge the blurred and unblurred texture in accordance with the CoC map
		mMergeBuffersShader->Execute(renderer->getDeviceContext(), renderer->getUnorderedAccessView(), mRenderTex->getShaderResourceView(), mBlurredRenderTex->getShaderResourceView(), mCoCMap->getShaderResourceView());
	}

	// Cleanup
	renderer->resetViewport();
	renderer->setBackBufferRenderTarget();
}

void CourseworkApp::showRenderTexture(ID3D11ShaderResourceView * texture)
{
	renderer->setZBuffer(false);

	// Ortho matrices
	XMMATRIX worldMatrix = mOrthoMesh.GetWorldMatrix();
	XMMATRIX viewMatrix = camera->getOrthoViewMatrix();
	XMMATRIX projectionMatrix = mRenderTex->getOrthoMatrix();

	mTextureShader->setShaderParameters(renderer->getDeviceContext(), worldMatrix, viewMatrix, projectionMatrix, texture);
	mOrthoMesh.Draw(renderer->getDeviceContext(), mTextureShader);

	UnsetPSShaderInputs(renderer->getDeviceContext());

	renderer->setZBuffer(true);
}

void CourseworkApp::initialiseInformation()
{
	std::ostringstream iss;

	iss << "- A yellow light moving up and down\n"
		<< "- A green light flying back and forth\n"
		<< "- A blue spotlight\n"
		<< "- A white directional light with dynamic direction, casting shadows\n"
		<< "- A plane with distance-based tessellation\n- A number of frustum cull-able shapes\n"
		<< "- A rain particle system\n"
		<< "- Full screen and depth of field gaussian blur\n"
		<< "- Fog\n- A heightmapped cube with wrong normals\n"
		<< "- A lonely billboarded quad";

	mInformation = iss.str();
}

void CourseworkApp::initialiseShaders(int screenWidth, int screenHeight, HWND hwnd)
{
	mColourShader = std::make_unique<ColourShader>(renderer->getDevice(), hwnd);
	mLightingShader = std::make_unique<LightingShader>(renderer->getDevice(), renderer->getDeviceContext(), hwnd);
	mWaveShader = std::make_unique<WaveShader>(renderer->getDevice(), renderer->getDeviceContext(), hwnd);
	mBillboardingShader = std::make_unique<BillboardingShader>(renderer->getDevice(), hwnd);
	mLightingShadowShader = std::make_unique<LightingShadowShader>(renderer->getDevice(), renderer->getDeviceContext(), hwnd);
	mBlurShader = std::make_unique<BlurShader>(renderer->getDevice(), screenWidth, screenHeight, hwnd);
	mTextureShader = std::make_unique<TextureShader>(renderer->getDevice(), hwnd);
	mInstanceShader = std::make_unique<InstanceShader>(renderer->getDevice(), renderer->getDeviceContext(), hwnd);

	WCHAR cocFilename[] = L"coc_cs.cso";
	mCoCShader = std::make_unique<InOutComputeShader>(renderer->getDevice(), screenWidth, screenHeight, cocFilename, hwnd);

	WCHAR mergeFilename[] = L"merge_cs.cso";
	mMergeBuffersShader = std::make_unique<InOutComputeShader>(renderer->getDevice(), screenWidth, screenHeight, mergeFilename, hwnd);
}

void CourseworkApp::initialiseRenderTextures(int screenWidth, int screenHeight)
{
	mShadowMap = std::make_unique<RenderTexture>(renderer->getDevice(), SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, SCREEN_NEAR, SCREEN_DEPTH);
	mRenderTex = std::make_unique<RenderTexture>(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	mBlurredRenderTex = std::make_unique<RenderTexture>(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
	mCoCMap = std::make_unique<RenderTexture>(renderer->getDevice(), screenWidth, screenHeight, SCREEN_NEAR, SCREEN_DEPTH);
}

void CourseworkApp::initialiseTextures()
{
	textureMgr->loadTexture("bunny", L"../res/bunny.png");
	textureMgr->loadTexture("bricks", L"../res/brick1.dds");
	textureMgr->loadTexture("cliff_d", L"../res/cliff_d.png");
	textureMgr->loadTexture("cliff_h", L"../res/cliff_h.png");
}

void CourseworkApp::initialiseMeshes(int screenWidth, int screenHeight)
{
	MeshManager::LoadMesh<CubeMesh>("Cube", renderer->getDevice(), renderer->getDeviceContext());
	MeshManager::LoadMesh<PlaneMesh>("Plane", renderer->getDevice(), renderer->getDeviceContext());
	MeshManager::LoadMesh<PointMesh>("Point", renderer->getDevice(), renderer->getDeviceContext());
	MeshManager::LoadMesh<SphereMesh>("Sphere", renderer->getDevice(), renderer->getDeviceContext());
	MeshManager::LoadMesh<OrthoMesh>("OrthoMesh", renderer->getDevice(), renderer->getDeviceContext(), screenWidth * 0.25f, screenHeight * 0.25f, screenWidth * 0.35f, screenHeight * 0.35f);
	MeshManager::LoadMesh<TessellatedPlane>("TessellatedPlane", renderer->getDevice(), renderer->getDeviceContext(), 100);
}

void CourseworkApp::initialiseMeshInstances()
{
	// Assign meshes to mesh instances
	mCubeMesh.SetMesh(MeshManager::GetMesh("Cube"));
	mBillboardPoint.SetMesh(MeshManager::GetMesh("Point"));
	mOrthoMesh.SetMesh(MeshManager::GetMesh("OrthoMesh"));
	mTessellatedPlaneMesh.SetMesh(MeshManager::GetMesh("TessellatedPlane"));

	// Initialise mesh properties
	mBillboardPoint.SetPosition(0.f, 2.f, 20.f);
	mCubeMesh.SetPosition(10, 1.f, 15.f);
	mTessellatedPlaneMesh.SetPosition(-20.f, -3.f, -20.f);

	initialiseCullableMeshInstances();
}

void CourseworkApp::initialiseCullableMeshInstances()
{
	// Spawn a bunch of meshes for use with frustum culling
	mMeshToInstance = MeshManager::GetMesh("Sphere");

	constexpr float SPACING = 8.f;
	for (int x = 0; x < NUM_MODELS_X; ++x)
	{
		for (int y = 0; y < NUM_MODELS_Y; ++y)
		{
			for (int z = 0; z < NUM_MODELS_Z; ++z)
			{
				int idx = (z * NUM_MODELS_X * NUM_MODELS_Y) + y * NUM_MODELS_X + x;
				mCullableMeshes[idx].SetMesh(mMeshToInstance);

				// Pick a pseudo-random position
				float offsetX = (rand() / (float) RAND_MAX) * 40.f;
				float offsetY = (rand() / (float) RAND_MAX) * 30.f;
				float offsetZ = (rand() / (float) RAND_MAX) * 25.f;

				mCullableMeshes[idx].SetPosition(offsetX + (x * SPACING),
												 offsetY + (y * SPACING),
												 -5.f + offsetZ + (z * SPACING));
			}
		}
	}
}

void CourseworkApp::initialiseLights()
{
	mLightingShader->setAmbient(0.1f, 0.1f, 0.1f);
	mLightingShader->setFogProperties(renderer->getDeviceContext(), mFogColour, mFogMin, mFogRange);

	mDirectionalLight = mLightingShader->addLight(LightingShader::DIRECTIONAL_LIGHT);
	mPointLightA = mLightingShader->addLight(LightingShader::POINT_LIGHT);
	mPointLightB = mLightingShader->addLight(LightingShader::POINT_LIGHT);
	mSpotlight = mLightingShader->addLight(LightingShader::SPOT_LIGHT);

	auto directionalLight = mLightingShader->getLight(mDirectionalLight);
	directionalLight->colour = { 0.4f, 0.4f, 0.4f, 1.f };
	directionalLight->direction = { 0.f, -0.5f, 0.5f, 0.f };

	// Calculate a position for the directional light for the light view matrix
	XMVECTOR direction = XMLoadFloat4(&directionalLight->direction);
	XMVECTOR position = -direction * 180.f;

	XMStoreFloat4(&directionalLight->position, position);

	directionalLight->specularPower = 16.f;

	auto pointLightA = mLightingShader->getLight(mPointLightA);
	pointLightA->colour = { 0.8f, 0.8f, 0.f, 1.f };
	pointLightA->position = { 10.f, 1.f, 10.f, 1.f };
	pointLightA->constAtt = 1.f;
	pointLightA->linearAtt = 0.035f;
	pointLightA->range = 256.f;
	pointLightA->specularPower = 128.f;

	auto pointLightB = mLightingShader->getLight(mPointLightB);
	pointLightB->colour = { 0.f, 0.8f, 0.f, 1.f };
	pointLightB->position = { 30.f, 3.f, 40.f, 1.f };
	pointLightB->constAtt = 1.f;
	pointLightB->linearAtt = 0.1f;
	pointLightB->range = 256.f;
	pointLightB->specularPower = 4.f;

	auto spotlight = mLightingShader->getLight(mSpotlight);
	spotlight->position = { 15.f, 4.f, 15.f, 1.f };
	spotlight->colour = { 0.9f, 0.3f, 0.1f, 1.f };
	spotlight->constAtt = 1.f;
	spotlight->linearAtt = 0.2f;
	spotlight->direction = { 0.25f, -0.5f, 0.25f, 0.f };
	spotlight->range = 256.f;
	spotlight->specularPower = 8.f;
	spotlight->spotAngle = XMConvertToRadians(30.f);

	// Make the shaders with lighting calculations use the same lights
	synchroniseLights();
}

void CourseworkApp::initialiseCullingMatrix(int screenWidth, int screenHeight)
{
	mAspectRatio = screenWidth / (float) screenHeight;

	updateCullingMatrix();
}

void CourseworkApp::initialiseBoundingVolume()
{
	std::vector<MeshInstance*> cullableMeshes(TOTAL_MODELS);

	for (int i = 0; i < TOTAL_MODELS; ++i)
		cullableMeshes[i] = &mCullableMeshes[i];

	mBoundingVolume = std::make_unique<BoundingVolume>(cullableMeshes);
}

void CourseworkApp::updateDirectionalLight()
{
	// Update directional (shadow casting) light's direction
	auto directionalLight = mLightingShader->getLight(mDirectionalLight);

	static const XMVECTOR BASE_DIR = XMVectorSet(0.f, -1.f, 0.f, 0.f);

	// Transform the direction vector
	XMVECTOR transformedDirection = XMVector3TransformNormal(BASE_DIR, XMMatrixRotationNormal(XMVectorSet(1.f, 0.f, 0.f, 0.f), sinf(mTotalTime) / 5.f));
	XMStoreFloat4(&directionalLight->direction, transformedDirection);

	XMVECTOR direction = XMLoadFloat4(&directionalLight->direction);
	XMVECTOR position = -direction * 180.f;

	XMStoreFloat4(&directionalLight->position, position);
}

void CourseworkApp::updatePointLights()
{
	auto pointLightA = mLightingShader->getLight(mPointLightA);
	pointLightA->position = {
		40.f + cosf(mTotalTime),
		1.f + (5.f * sinf(mTotalTime)),
		30.f + sinf(mTotalTime),
		1.f
	};

	auto pointLightB = mLightingShader->getLight(mPointLightB);
	pointLightB->position = {
		5.f,
		pointLightB->position.y,
		20.f + (20.f * sinf(5.f * mTotalTime)),
		1.f
	};
}

void CourseworkApp::updateSpotlight()
{
	auto spotlight = mLightingShader->getLight(mSpotlight);
	spotlight->direction = {
		sinf(mTotalTime * 1.4f),
		-1.f,
		cosf(mTotalTime * 0.75f),
		0.f
	};
}

void CourseworkApp::synchroniseLights()
{
	// Make the lights for all shaders the same to give a consistent result
	mWaveShader->update(renderer->getDeviceContext(), *mLightingShader);
	mLightingShadowShader->update(renderer->getDeviceContext(), *mLightingShader);
	mInstanceShader->update(renderer->getDeviceContext(), *mLightingShader);
}

void CourseworkApp::updateCullingMatrix()
{
	XMMATRIX cullingMatrix = XMMatrixPerspectiveFovLH(mCullingFov, mAspectRatio, SCREEN_NEAR, SCREEN_DEPTH);
	XMStoreFloat4x4(&mCullingMatrix, cullingMatrix);
}

void CourseworkApp::createShadowMap(const LightingShader::ShaderLight& light)
{
	// Create light's view and projection matrices
	XMMATRIX viewMatrix = mLightingShader->generateLightViewMatrix(mDirectionalLight);
	XMMATRIX projectionMatrix = XMMatrixOrthographicLH(LIGHT_PROJECTION_FRUSTUM_DIM, LIGHT_PROJECTION_FRUSTUM_DIM, SCREEN_NEAR, SCREEN_DEPTH);

	// Set render target
	mShadowMap->setRenderTarget(renderer->getDeviceContext());
	mShadowMap->clearRenderTarget(renderer->getDeviceContext(), CLEAR_COLOUR);

	// Disable colour writes
	// Since the fragment shader is bound, rasterisation will still occur, but it will render to the depth buffer only
	mShadowMap->setColourWrites(renderer->getDeviceContext(), false);

	renderScene(viewMatrix, projectionMatrix, true);

	// Clean up
	mShadowMap->setColourWrites(renderer->getDeviceContext(), true);

	renderer->resetViewport();
	renderer->setBackBufferRenderTarget();
}

