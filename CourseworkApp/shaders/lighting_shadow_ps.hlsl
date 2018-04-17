#include "lighting_header.hlsl"

// Must correspond with the buffer created from the application
#define SHADOW_MAP_SIZE		2048

#define SHADOW_MAP_STRIDE	1.f / SHADOW_MAP_SIZE

// Minimise shadow acne
#define SHADOW_BIAS			0.00005f 

// t0 defined in lighting_header
Texture2D gShadowMap : register(t1);
// s0 defined in lighting_header
SamplerComparisonState gComparisonSampler : register(s1);

struct Input
{
	float3 positionW : POSITION;
	float3 cameraDirW : CAMERA;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 positionLH : POSITION_RELATIVE_TO_LIGHT;
};

float CalculateShadowFactor(float2 samplePoint, float depth)
{
	float shadowFactor = 0.f;

	static const float dx = SHADOW_MAP_STRIDE;
	static const float2 offsets[9] =
	{
		{-dx, -dx }, { 0.f, -dx }, {dx, -dx },
		{-dx, 0.f }, { 0.f, 0.f }, {dx, 0.f },
		{-dx,  dx }, { 0.f,  dx }, {dx,  dx }
	};

	// Sample shadow map using comparison sampler with a 3x3 box kernel
	// for a smoother result
	[unroll]
	for (int i = 0; i < 9; ++i)
	{
		shadowFactor += gShadowMap.SampleCmpLevelZero(gComparisonSampler, samplePoint + offsets[i], depth);
	}
	shadowFactor /= 9.f;

	return shadowFactor;
}

float4 main(Input input) : SV_TARGET
{
	float4 lightColour = 0.f;

	float distance = length(input.cameraDirW);
	float fogCoef = saturate((distance - gFogMin) / gFogRange);

	//// Discard fragments hidden completely by the fog
	//clip(0.99f - fogCoef);

	// If the fragment is not outside of fog range, do lighting calculations
	if (fogCoef < 1.f)
	{
		// Finish light's point of view projection
		float3 positionLUV = input.positionLH / input.positionLH.w;

		// Sample depth and apply bias
		float depth = positionLUV.z - SHADOW_BIAS;

		float shadowFactor = CalculateShadowFactor(positionLUV.xy, depth);

		// Calculate lighting
		float3 viewDir = input.cameraDirW / distance;
		LightingResult lighting = CalcLighting(input.positionW.xyz, viewDir, input.normal, shadowFactor);

		// Combine with material
		float4 sampleColour = gTexture.Sample(gSampler, input.tex);
		lightColour = (gAmbient + lighting.diffuse + lighting.specular) * sampleColour;
	}

	// Blend fragment colour and fog colour
	float4 finalColour = lerp(lightColour, gFogColour, fogCoef);

	return finalColour;
}