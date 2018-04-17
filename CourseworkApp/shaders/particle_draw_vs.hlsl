// Vertex shader used when drawing the particle system

#include "particle_header.hlsl"

struct VertOut
{
	float3 positionW : POSITION;
	uint type : TYPE;
};

VertOut main(Particle vin)
{
	VertOut vout;
	
	// Calculate the particle's position based on its age
	float age = vin.age;
	vout.positionW = 0.5f * age * age * gAccelerationW + age * vin.initialVelocityW + vin.initialPositionW;

	vout.type = vin.type;
	return vout;
}
