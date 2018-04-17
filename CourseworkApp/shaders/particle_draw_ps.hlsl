struct GeoOut
{
	float4 positionH : SV_POSITION;
	float2 tex : TEXCOORD;
};

float4 main(GeoOut pIn) : SV_TARGET
{
	return float4(1.f, 0.65f, 1.f, 1.f);
}