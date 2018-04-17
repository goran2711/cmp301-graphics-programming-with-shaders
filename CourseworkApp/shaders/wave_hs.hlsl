// Hull shader which does tessellation based on the camera's distance from each control point

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

cbuffer TessellationProperties : register(b2)
{
	// Smallest tessellation factor (0)
	int gMinTess;
	// Largest tessellation factor (6)
	int gMaxTess;
	// Distance with the smallest tessellation factor
	float gMinDistance;
	// Distance with the largest tessellation factor
	float gMaxDistance;
};

struct InputVS
{
	float4 positionL : POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
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

float CalculateTessFactor(float4 positionL)
{
	// Get the control point position in the same coordinate space as the camera
	float3 positionW = mul(positionL, gWorldMatrix).xyz;

	// Calculate the distance to the camera
	float distanceToCameraW = distance(gCameraPositionW, positionW);

	// Calculate the tessellation factor based on the distance to the camera
	const float range = gMinDistance - gMaxDistance;

	float s = saturate((distanceToCameraW - gMaxDistance) / range);
	float tessFactor = lerp(gMaxTess, gMinTess, s);

	// Exponential change in tessellation detail
	return pow(2.f, tessFactor);
}

OutputCHS ConstantHullShader(InputPatch<InputVS, 3> input, uint patchID : SV_PrimitiveID)
{
	OutputCHS output;

	// Calculate edge tessellation factors
	float t0 = CalculateTessFactor(input[0].positionL);
	float t1 = CalculateTessFactor(input[1].positionL);
	float t2 = CalculateTessFactor(input[2].positionL);

	output.edges[0] = (t1 + t2) / 2.f;
	output.edges[1] = (t2 + t0) / 2.f;
	output.edges[2] = (t0 + t1) / 2.f;

	// Calculate centroid tessellation factor
	float3 centre = (input[0].positionL + input[1].positionL + input[2].positionL) / 3.f;

	float c0 = CalculateTessFactor(float4(centre, 1.f));

	output.inside = c0;

	return output;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHullShader")]
OutputCPHS main(InputPatch<InputVS, 3> input, uint controlPointID : SV_OutputControlPointID, uint patchID : SV_PrimitiveID)
{
	OutputCPHS output;

	// Pass-through control point hull shader
	output.positionL = input[controlPointID].positionL;
	output.tex = input[controlPointID].tex;
	output.normal = input[controlPointID].normal;

	return output;
}