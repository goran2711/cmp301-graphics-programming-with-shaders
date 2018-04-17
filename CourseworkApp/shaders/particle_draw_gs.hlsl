// Geometry shader that creates a line from a point to represent a rain drop

#include "particle_header.hlsl"

struct VertOut
{
	float3 posW : POSITION;
	uint type : TYPE;
};

struct GeoOut
{
	float4 posH : SV_POSITION;
	float2 tex : TEXCOORD;
};

[maxvertexcount(2)]
void main(point VertOut geoIn[1], inout LineStream<GeoOut> geoOut)
{
	// do not render emitters
	if (geoIn[0].type == EMITTER)
		return;

	// Vertex positions
	float3 p0 = geoIn[0].posW;
	float3 p1 = p0 + DROP_LENGTH * gAccelerationW;

	// Build vertices
	GeoOut v0;
	v0.posH = mul(float4(p0, 1.f), gViewProjection);
	v0.tex = 0.f;
	geoOut.Append(v0);

	GeoOut v1;
	v1.posH = mul(float4(p1, 1.f), gViewProjection);
	v1.tex = 1.f;
	geoOut.Append(v1);
}
