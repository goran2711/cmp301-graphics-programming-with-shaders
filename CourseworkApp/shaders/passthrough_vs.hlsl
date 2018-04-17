// Pass-through vertex shader

struct InputVS
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
};

InputVS main(InputVS input)
{
	input.positionL.w = 1.f;
	return input;
}