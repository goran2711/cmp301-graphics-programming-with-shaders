cbuffer MatrixBuffer : register(b0)
{
	row_major matrix gViewMatrix;
	row_major matrix gProjectionMatrix;
};

cbuffer CameraBuffer : register(b1)
{
	float3 gCameraPositionW;
};

struct Input
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;

	// Instance specific data
	row_major matrix worldMatrix : WORLD;
};

struct Output
{
	float3 positionW : POSITION;
	float3 cameraVecW : CAMERA;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 positionH : SV_POSITION;
};

Output main(Input input)
{
	input.positionL.w = 1.f;

	Output output;

	output.positionW = mul(input.positionL, input.worldMatrix).xyz;

	output.cameraVecW = gCameraPositionW - output.positionW;

	output.positionH = mul(float4(output.positionW, 1.f), gViewMatrix);
	output.positionH = mul(output.positionH, gProjectionMatrix);

	output.tex = input.tex;

	output.normal = normalize(mul(input.normal, (float3x3)input.worldMatrix));

	return output;
}