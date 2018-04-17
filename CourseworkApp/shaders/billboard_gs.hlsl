// Geometry shader that creates a billboarded quad from a point

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

cbuffer FixedBuffer : register(b2)
{
	static const float2 gOffsets[4] =
	{
		{ -1.f, -1.f },	// Top left
		{ -1.f,  1.f },	// Bottom left
		{  1.f, -1.f },	// Top right
		{  1.f,  1.f }	// Bottom right
	};
	static const float3 gUp = float3(0.f, 1.f, 0.f);
};

struct GSIn
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
};

struct GSOut
{
	float4 positionH : SV_POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
};

[maxvertexcount(4)]
void main(point GSIn input[1], inout TriangleStream<GSOut> output)
{
	float4 positionW = mul(input[0].positionL, gWorldMatrix);

	// Calculate billboard axes
	float3 forward = normalize(gCameraPositionW - positionW).xyz;
	float3 right = normalize(cross(gUp, forward));

	// Create four vertices
	for (int i = 0; i < 4; ++i)
	{
		GSOut vertex;

		vertex.positionH = float4(positionW.xyz + right * gOffsets[i].x + gUp * gOffsets[i].y, 1.f);
		vertex.positionH = mul(vertex.positionH, gViewMatrix);
		vertex.positionH = mul(vertex.positionH, gProjectionMatrix);

		vertex.tex = ((gOffsets[i] * float2(1.f, -1.f)) / 2.f) + 0.5f;
		vertex.normal = forward;

		output.Append(vertex);
	}
}