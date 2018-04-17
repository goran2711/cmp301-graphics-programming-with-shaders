Texture2D gHeightMap : register(t0);
SamplerState gSampler : register(s0);

cbuffer MatrixBuffer : register(b0)
{
	row_major matrix gWorldMatrix;
	row_major matrix gViewMatrix;
	row_major matrix gProjectionMatrix;
};

cbuffer CameraBuffer : register(b1)
{
	float3 gCameraPositionW;
};

cbuffer MaterialProperties : register(b2)
{
	bool gUseHeightMap;
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
	float4 positionH : SV_POSITION;
};

Output main(Input input)
{
	input.positionL.w = 1.f;

	if (gUseHeightMap)
	{
		static const float scale = 0.2f;
		float offset = gHeightMap.SampleLevel(gSampler, input.tex, 0).r * scale;

		// Do not displace the very edges of the shape
		if (input.tex.x != 0.f && input.tex.x < 0.96f && input.tex.y != 0.f && input.tex.y < 0.96f)
		input.positionL += float4(input.normal, 0.f) * offset;
	}

	Output output;

	output.positionW = mul(input.positionL, gWorldMatrix).xyz;

	output.cameraVecW = gCameraPositionW - output.positionW;

	output.positionH = mul(float4(output.positionW, 1.f), gViewMatrix);
	output.positionH = mul(output.positionH, gProjectionMatrix);

	output.tex = input.tex;

	output.normal = normalize(mul(input.normal, (float3x3)gWorldMatrix));

	return output;
}