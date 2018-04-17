#include "lighting_header.hlsl"

struct Input
{
	float3 positionW : POSITION;
	float3 cameraDirW : CAMERA;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

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
		// Calculate lighting
		float3 viewDir = input.cameraDirW / distance;
		LightingResult lighting = CalcLighting(input.positionW.xyz, viewDir, input.normal);

		// Combine with material
		float4 sampleColour = gTexture.Sample(gSampler, input.tex);
		lightColour = (gAmbient + lighting.diffuse + lighting.specular) * sampleColour;
	}

	// Blend fragment colour and fog colour
	float4 finalColour = lerp(lightColour, gFogColour, fogCoef);

	return finalColour;
}