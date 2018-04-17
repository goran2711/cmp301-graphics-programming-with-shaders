#include "MeshInstance.h"

XMMATRIX MeshInstance::GetWorldMatrix() const
{
	XMVECTOR scale = XMLoadFloat3(&mScale);
	XMVECTOR rotation = XMLoadFloat4(&mRotation);
	XMVECTOR position = XMLoadFloat3(&mPosition);

	XMMATRIX worldMatrix = XMMatrixIdentity();
	worldMatrix *= XMMatrixScalingFromVector(scale);
	worldMatrix *= XMMatrixRotationQuaternion(rotation);
	worldMatrix *= XMMatrixTranslationFromVector(position);

	return worldMatrix;
}
