// Domain shader with vertex manipulation

cbuffer MatrixBuffer : register(b0)
{
	row_major matrix gWorldMatrix;
	row_major matrix gViewMatrix;
	row_major matrix gProjectionMatrix;

	row_major matrix gShadowTransformMatrix;
};

cbuffer CameraBuffer : register(b1)
{
	float3 gCameraPositionW;
};

cbuffer TimeBuffer : register(b2)
{
	float gTime;
};

struct OutputCPHS
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
};

struct OutputCHS
{
	float edges[3] : SV_TessFactor;
	float inside : SV_InsideTessFactor;
};

struct OutputDS
{
	float3 positionW : POSITION;
	float3 cameraDirW : CAMERA;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float4 positionLH : POSITION_RELATIVE_TO_LIGHT;
	float4 positionH : SV_POSITION;
};

float GetDisplacement(float2 coordinate)
{
	static const float height = 1.f;
	static const float freq = 1.f;

	// determine displacement with some random algorithm
	return height * sin(coordinate.y + freq * gTime) + (0.3f * cos(coordinate.x + gTime));
}

float3 CalculateNormal(float3 positionL)
{
	float2 right = positionL.xz + float2(1.f, 0.f);
	float2 left = positionL.xz + float2(-1.f, 0.f);

	float2 top = positionL.xz + float2(0.f, 1.f);
	float2 bottom = positionL.xz + float2(0.f, -1.f);

	// Calculate the slopes between the adjacent triangles
	float3 horizontalTangent = { 2.f, GetDisplacement(right) - GetDisplacement(left), 0.f };
	float3 verticalTangent = { 0.f, GetDisplacement(top) - GetDisplacement(bottom), 2.f };

	// Normalised cross product gives us an accurate enough normal
	return normalize(cross(verticalTangent, horizontalTangent));
}

[domain("tri")]
OutputDS main(OutputCHS inputCPHS, float3 uvw : SV_DomainLocation, const OutputPatch<OutputCPHS, 3> input)
{
	OutputDS output;

	// Calculate tessellated vertex' barycentric coordinate
	float4 positionL = input[0].positionL * uvw.x + input[1].positionL * uvw.y + input[2].positionL * uvw.z;
	float3 baseNormal = normalize(input[0].normal * uvw.x + input[1].normal * uvw.y + input[2].normal * uvw.z);

	// Displace
	positionL.y = GetDisplacement(positionL.xz);

	positionL.w = 1.f;

	// Transform to clip space
	output.positionW = mul(positionL, gWorldMatrix);

	// Store direction to camera for lighting calculations
	output.cameraDirW = gCameraPositionW - output.positionW;

	output.positionH = mul(float4(output.positionW, 1.f), gViewMatrix);
	output.positionH = mul(output.positionH, gProjectionMatrix);

	// Calculate texture coord and normal
	output.tex = input[0].tex * uvw.x + input[1].tex * uvw.y + input[2].tex * uvw.z;
	output.normal = mul(CalculateNormal(positionL), (float3x3)gWorldMatrix);

	// Calculate the vertex' position relative to the shadow-casting light
	output.positionLH = mul(float4(output.positionW, 1.f), gShadowTransformMatrix);

	return output;
}