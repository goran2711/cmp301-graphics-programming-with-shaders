// Light class
// Holds data that represents a single light source
#include "light.h"

// create view matrix, based on light position and lookat. Used for shadow mapping.
void Light::generateViewMatrix()
{
	// default up vector
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	// Create the view matrix from the three vectors.
	viewMatrix = XMMatrixLookAtLH(position, lookAt, up);
}

// Create a projection matrix for the (point) light source. Used in shadow mapping.
void Light::generateProjectionMatrix(float screenNear, float screenFar)
{
	float fieldOfView, screenAspect;

	// Setup field of view and screen aspect for a square light source.
	fieldOfView = (float)XM_PI / 2.0f;
	screenAspect = 1.0f;

	// Create the projection matrix for the light.
	projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, screenNear, screenFar);
}

// Create orthomatrix for (directional) light source. Used in shadow mapping.
void Light::generateOrthoMatrix(float screenWidth, float screenHeight, float near, float far)
{
	orthoMatrix = XMMatrixOrthographicLH(screenWidth, screenHeight, near, far);
}

void Light::setAmbientColour(float red, float green, float blue, float alpha)
{
	ambientColour = XMFLOAT4(red, green, blue, alpha);
}

void Light::setDiffuseColour(float red, float green, float blue, float alpha)
{
	diffuseColour = XMFLOAT4(red, green, blue, alpha);
}

void Light::setDirection(float x, float y, float z)
{
	direction = XMFLOAT3(x, y, z);
}

void Light::setSpecularColour(float red, float green, float blue, float alpha)
{
	specularColour = XMFLOAT4(red, green, blue, alpha);
}

void Light::setSpecularPower(float power)
{
	specularPower = power;
}

void Light::setPosition(float x, float y, float z)
{
	position = XMVectorSet(x, y, z, 1.0f);
}

XMFLOAT4 Light::getAmbientColour() const
{
	return ambientColour;
}

XMFLOAT4 Light::getDiffuseColour() const
{
	return diffuseColour;
}


XMFLOAT3 Light::getDirection() const
{
	return direction;
}

XMFLOAT4 Light::getSpecularColour() const
{
	return specularColour;
}


float Light::getSpecularPower() const
{
	return specularPower;
}

XMFLOAT3 Light::getPosition() const
{
	XMFLOAT3 temp(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position));
	return temp;
}

void Light::setLookAt(float x, float y, float z)
{
	lookAt = XMVectorSet(x, y, z, 1.0f);
}

XMMATRIX Light::getViewMatrix() const
{
	return viewMatrix;
}

XMMATRIX Light::getProjectionMatrix() const
{
	return projectionMatrix;
}

XMMATRIX Light::getOrthoMatrix() const
{
	return orthoMatrix;
}