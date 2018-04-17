// Common data between all particle system shaders

#define EMITTER	0
#define FLARE	1

#define EMIT_INTERVAL	0.0001f
#define EMIT_NUM		75

#define EMIT_RADIUS		25.f
#define EMIT_HEIGHT		20.f

#define MAX_AGE			1.4f

#define WIND_INTENSITY	0.f
#define RAINDROP_FORCE	10.f

#define DROP_LENGTH		0.07f

// A 1D texture full of random numbers generated on the CPU
Texture1D gRandomTexture : register(t0);

SamplerState gLinearSampler : register(s0);

cbuffer Fixed : register(b0)
{
	float3 gAccelerationW;
};

cbuffer PerFrame : register(b1)
{
	float3 gEyePositionW;
	float gTotalTime;

	float3 gEmitPositionW;
	float gFrameTime;

	float3 gEmitDirectionW;
	float _padding;

	row_major matrix gViewProjection;
};

struct Particle
{
	float3 initialPositionW : POSITION;
	float3 initialVelocityW : VELOCITY;
	float2 sizeW : SIZE;
	float age : AGE;
	uint type : TYPE;
};