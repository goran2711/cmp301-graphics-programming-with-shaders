// Header file containing lighting calculations that are shared between lighting_ps and lighting_shadow_ps

#define MAX_LIGHTS 8

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Light
{
	float4 positionW;
	float4 directionW;
	float4 colour;

	float specularPower;
	float range;
	float spotAngle;
	float constAtt;

	float linearAtt;
	float quadraticAtt;
	int type;
	bool enabled;
};

struct LightingResult
{
	float4 diffuse;
	float4 specular;
};

cbuffer LightProperties : register(b0)
{
	float4 gAmbient;
	Light gLights[MAX_LIGHTS];
};

cbuffer FogProperties : register(b1)
{
	float4 gFogColour;
	float gFogMin;
	float gFogRange;
};

cbuffer MaterialProperties : register(b2)
{
	bool gUseHeightMap;
};

float4 CalcDiffuse(Light light, float3 lightDir, float3 normal)
{
	float NdotL = max(0.f, dot(normal, lightDir));
	return light.colour * NdotL;
}

float4 CalcSpecular(Light light, float3 viewDir, float3 lightDir, float3 normal)
{
	// Blinn-Phong
	float3 halfway = normalize(lightDir + viewDir);
	float NdotH = max(0.f, dot(normal, halfway));
	return light.colour * pow(NdotH, light.specularPower);
}

float CalcAttenuation(Light light, float dist)
{
	return 1.f / (light.constAtt + light.linearAtt * dist + light.quadraticAtt * dist * dist);
}

LightingResult CalcDirectionalLight(Light light, float3 viewDir, float3 normal)
{
	LightingResult result = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

	float3 lightDir = -normalize(light.directionW);

	result.diffuse = CalcDiffuse(light, lightDir, normal);
	result.specular = CalcSpecular(light, viewDir, lightDir, normal);

	return result;
}

LightingResult CalcPointLight(Light light, float3 viewDir, float3 positionW, float3 normal)
{
	LightingResult result = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

	float3 lightDir = (light.positionW - positionW);
	float distance = length(lightDir);

	// Do not go any further if the light is too far away
	if (distance >= light.range)
		return result;

	lightDir /= distance;

	float att = CalcAttenuation(light, distance);

	result.diffuse = CalcDiffuse(light, lightDir, normal) * att;
	result.specular = CalcSpecular(light, viewDir, lightDir, normal) * att;

	return result;
}

float CalcSpotlightCone(Light light, float3 lightDir)
{
	// Angle where light intensity will be the lowest
	float spotMinAngle = cos(light.spotAngle);

	// Angle where light intensity will be 100%
	float spotMaxAngle = (spotMinAngle + 1.f) / 2.f;

	// Current angle
	float cosAngle = dot(-light.directionW.xyz, lightDir);

	// Smoothstep interpolation for a more interesting result
	return smoothstep(spotMinAngle, spotMaxAngle, cosAngle);
}

LightingResult CalcSpotlight(Light light, float3 viewDir, float3 positionW, float3 normal)
{
	LightingResult result = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

	float3 lightDir = (light.positionW - positionW);
	float distance = length(lightDir);

	// Don't continue if the light is too far away
	if (distance >= light.range)
		return result;

	lightDir /= distance;

	float att = CalcAttenuation(light, distance);

	// Intensity of the light based on the fragment's position relative to the spotlight and it's light direction
	float spotIntensity = CalcSpotlightCone(light, lightDir);

	result.diffuse = CalcDiffuse(light, lightDir, normal) * att * spotIntensity;
	result.specular = CalcSpecular(light, viewDir, lightDir, normal) * att * spotIntensity;

	return result;
}

LightingResult CalcLighting(float3 positionW, float3 viewDir, float3 normal, float shadowFactor = 1.f)
{
	LightingResult total = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

	[unroll]
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		if (!gLights[i].enabled)
			continue;

		LightingResult result = { { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } };

		switch (gLights[i].type)
		{
			case DIRECTIONAL_LIGHT:
				result = CalcDirectionalLight(gLights[i], viewDir, normal);
				break;
			case POINT_LIGHT:
				result = CalcPointLight(gLights[i], viewDir, positionW, normal);
				break;
			case SPOT_LIGHT:
				result = CalcSpotlight(gLights[i], viewDir, positionW, normal);
				break;
		}

		total.diffuse += result.diffuse * shadowFactor;
		total.specular += result.specular * shadowFactor;
	}

	return total;
}
