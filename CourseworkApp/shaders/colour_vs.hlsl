// colour vertex shader
// Simple geometry pass
// texture coordinates and normals will be ignored.

cbuffer MatrixBuffer : register(b0)
{
	matrix gWorldMatrix;
	matrix gViewMatrix;
	matrix gProjectionMatrix;
};

struct InputType
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct OutputType
{
	float4 positionH : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

OutputType main(InputType input)
{
	OutputType output;
	
	// Change the position vector to be 4 units for proper matrix calculations.
	input.positionL.w = 1.0f;


	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.positionH = mul(input.positionL, gWorldMatrix);
	output.positionH = mul(output.positionH, gViewMatrix);
	output.positionH = mul(output.positionH, gProjectionMatrix);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Store normals for the pixel shader
	output.normal = mul(input.normal, (float3x3)gWorldMatrix);
	output.normal = normalize(output.normal);

	return output;
}