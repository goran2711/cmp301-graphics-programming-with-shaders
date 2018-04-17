// Class that has a mesh and a transform matrix
// Provides an interface for manipulating scale, rotation and translation

#pragma once
#include "../DXFramework/BaseMesh.h"
#include <DirectXMath.h>
#include <memory>
#include <DirectXCollision.h>

using namespace DirectX;

class MeshInstance
{
public:
	~MeshInstance() = default;

	// Data must be sent to the shader before calling this function
	// ShaderType& must be a pointer type
	template <typename ShaderType>
	void Draw(ID3D11DeviceContext* context, ShaderType& shader)
	{
		mMesh->sendData(context);
		shader->render(context, mMesh->getIndexCount());
	}

	//
	//// Setters
	//

	void SetMesh(BaseMesh* mesh) { mMesh = mesh; }

	void SetBoundingBox(const BoundingBox& boundingBox) { mBoundingBox = boundingBox; }

	void SetScale(float x, float y, float z) { mScale = { x, y, z }; }
	void SetScale(FXMVECTOR scale) { XMStoreFloat3(&mScale, scale); }

	void SetRotation(FXMVECTOR rotation) { XMStoreFloat4(&mRotation, rotation); }

	void SetPositionX(float x) { mPosition.x = x; }
	void SetPositionY(float y) { mPosition.y = y; }
	void SetPositionZ(float z) { mPosition.z = z; }
	void SetPosition(float x, float y, float z) { mPosition = { x, y, z }; }
	void SetPosition(FXMVECTOR position) { XMStoreFloat3(&mPosition, position); }

	//
	//// Getters
	//

	BoundingBox& GetBoundingBox() { return mBoundingBox; }
	const BoundingBox& GetBoundingBox() const { return mBoundingBox; }

	XMFLOAT3 GetScale() const { return mScale; }
	XMVECTOR GetScaleXM() const { return XMLoadFloat3(&mScale); }

	XMFLOAT4 GetRotation() const { return mRotation; }
	XMVECTOR GetRotationXM() const { return XMLoadFloat4(&mRotation); }

	XMFLOAT3 GetPosition() const { return mPosition; }
	XMVECTOR GetPositionXM() const { return XMLoadFloat3(&mPosition); }

	XMFLOAT4X4 GetTransform() const { return mTransform; }
	XMMATRIX GetTransformXM() const { return XMLoadFloat4x4(&mTransform); }

	XMMATRIX GetWorldMatrix() const;

private:
	BaseMesh* mMesh = nullptr;
	BoundingBox mBoundingBox;

	XMFLOAT3 mPosition;
	XMFLOAT4 mRotation = { 0.f, 0.f, 0.f, 1.f };
	XMFLOAT3 mScale = { 1.f, 1.f, 1.f };
	XMFLOAT4X4 mTransform;
};

