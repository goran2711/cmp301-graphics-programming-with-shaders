// Texture pixel/fragment shader

Texture2D gInput : register(t0);
SamplerState gSampler : register(s0);

struct InputType
{
	float4 positionH : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};


float4 main(InputType input) : SV_TARGET
{
	return gInput.Sample(gSampler, input.tex);
}