cbuffer MatrixBuffer : register(b0)
{
	row_major matrix gWorldMatrix;
	row_major matrix gViewMatrix;
	row_major matrix gProjectionMatrix;
	
	row_major matrix gShadowMatrix;
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
};

struct Output
{
	float3 positionW : POSITION;
	float3 cameraVecW : CAMERA;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 positionLH : POSITION_RELATIVE_TO_LIGHT;
	float4 positionH : SV_POSITION;
};

Output main(Input input)
{
	input.positionL.w = 1.f;

	Output output;

	output.positionW = mul(input.positionL, gWorldMatrix).xyz;

	output.cameraVecW = gCameraPositionW - output.positionW;

	output.positionH = mul(float4(output.positionW, 1.f), gViewMatrix);
	output.positionH = mul(output.positionH, gProjectionMatrix);

	output.tex = input.tex;

	output.normal = normalize(mul(input.normal, (float3x3)gWorldMatrix));

	// Calculate the vertex' position relative to the shadow-casting light
	output.positionLH = mul(input.positionL, gWorldMatrix);
	output.positionLH = mul(output.positionLH, gShadowMatrix);

	return output;
}