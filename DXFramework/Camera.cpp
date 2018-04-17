// Camera class
// Represents a single 3D camera with basic movement.
#include "camera.h"

// Configure defaul camera (including positions, rotation and ortho matrix)
Camera::Camera()
{
	position = XMFLOAT3(0.f, 0.f, 0.f);
	rotation = XMFLOAT3(0.f, 0.f, 0.f);

	speed = 5.f;
	lookSpeed = 4.0f;
	turnSpeed = 25.f;

	// Generate ortho matrix
	XMVECTOR up, position, lookAt;
	up = XMVectorSet(0.0f, 1.0, 0.0, 1.0f);
	position = XMVectorSet(0.0f, 0.0, -10.0, 1.0f);
	lookAt = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0);
	orthoMatrix = XMMatrixLookAtLH(position, lookAt, up);
}

// Store frame/delta time.
void Camera::setFrameTime(float t)
{
	frameTime = t;
}

void Camera::setSpeed(float newSpeed)
{
	speed = newSpeed;
}

void Camera::setPosition(float lx, float ly, float lz)
{
	position.x = lx;
	position.y = ly;
	position.z = lz;
}

void Camera::setRotation(float lx, float ly, float lz)
{
	rotation.x = lx;
	rotation.y = ly;
	rotation.x = lz;
}

float Camera::getSpeed() const
{
	return speed;
}

XMFLOAT3 Camera::getPosition() const
{
	return position;
}

XMVECTOR Camera::getRotation() const
{
	XMVECTOR rot;
	rot = XMLoadFloat3(&rotation);
	return rot;
}

// Re-calucation view Matrix.
void Camera::update()
{
	XMVECTOR up, positionv, lookAt;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;
	
	// Setup the vectors
	up = XMVectorSet(0.0f, 1.0, 0.0, 1.0f);
	positionv = XMLoadFloat3(&position);
	lookAt = XMVectorSet(0.0, 0.0, 1.0f, 1.0f);
	
	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = XMConvertToRadians(rotation.x);
	yaw = XMConvertToRadians(rotation.y);
	roll = XMConvertToRadians(rotation.z);

	// Create the rotation matrix from the yaw, pitch, and roll values.
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
	lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
	up = XMVector3TransformCoord(up, rotationMatrix);
	
	// Translate the rotated camera position to the location of the viewer.
	lookAt = positionv + lookAt;

	// Finally create the view matrix from the three updated vectors.
	viewMatrix = XMMatrixLookAtLH(positionv, lookAt, up);
}


XMMATRIX Camera::getViewMatrix() const
{
	return viewMatrix;
}

XMMATRIX Camera::getOrthoViewMatrix() const
{
	return orthoMatrix;
}

void Camera::moveForward()
{
	float radians;

	// Calculate the forward movement based on the frame time
	float movDelta = speed * frameTime;
	
	// Convert degrees to radians.
	radians = XMConvertToRadians(rotation.y);

	// Update the position.
	position.x += sinf(radians) * movDelta;
	position.z += cosf(radians) * movDelta;
}


void Camera::moveBackward()
{
	float radians;

	// Calculate the backward movement based on the frame time
	float movDelta = speed * frameTime;

	// Convert degrees to radians.
	radians = XMConvertToRadians(rotation.y);

	// Update the position.
	position.x-= sinf(radians) * movDelta;
	position.z -= cosf(radians) * movDelta;
}


void Camera::moveUpward()
{
	// Calculate the backward movement based on the frame time
	float movDelta = speed * frameTime;
	
	// Update the height position.
	position.y += movDelta;
}


void Camera::moveDownward()
{
	// Calculate the backward movement based on the frame time
	float movDelta = speed * frameTime;

	// Update the height position.
	position.y -= movDelta;
}


void Camera::turnLeft()
{
	// Calculate the left turn movement based on the frame time 
	float movDelta = turnSpeed * frameTime;
	
	// Update the rotation.
	rotation.y -= movDelta;

	// Keep the rotation in the 0 to 360 range.
	if (rotation.y < 0.0f)
	{
		rotation.y += 360.0f;
	}
}


void Camera::turnRight()
{
	// Calculate the right turn movement based on the frame time
	float movDelta = turnSpeed * frameTime;
	
	// Update the rotation.
	rotation.y += movDelta;

	// Keep the rotation in the 0 to 360 range.
	if (rotation.y > 360.0f)
	{
		rotation.y -= 360.0f;
	}

}


void Camera::turnUp()
{
	// Calculate the upward turn movement based on the frame time
	float movDelta = turnSpeed * frameTime;
	
	// Update the rotation.
	rotation.x -= movDelta;

	// Keep the rotation maximum 90 degrees.
	rotation.x = fminf(rotation.x, 90.f);
}


void Camera::turnDown()
{
	// Calculate the downward rotation movement based on the frame time
	float movDelta = turnSpeed * frameTime;

	// Update the rotation.
	rotation.x += movDelta;

	// Keep the rotation maximum 90 degrees.
	rotation.x = fmaxf(rotation.x, -90.f);
}


void Camera::turn(int x, int y)
{
	// Update the rotation.
	rotation.y += (float)x/lookSpeed;// m_speed * x;

	rotation.x += (float)y/lookSpeed;// m_speed * y;
}

void Camera::strafeRight()
{
	float radians;

	// Calculate the forward movement based on the frame time
	float movDelta = speed * frameTime;

	// Convert degrees to radians.
	radians = XMConvertToRadians(rotation.y);

	// Update the position.
	position.z -= sinf(radians) * movDelta;
	position.x += cosf(radians) * movDelta;

}

void Camera::strafeLeft()
{
	float radians;

	// Calculate the forward movement based on the frame time
	float movDelta = speed * frameTime;

	// Convert degrees to radians.
	radians = XMConvertToRadians(rotation.y);

	// Update the position.
	position.z += sinf(radians) * movDelta;
	position.x -= cosf(radians) * movDelta;
}