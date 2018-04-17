// Shader that merges a blurred and unblurred texture (usually frame buffer) in accordance with the circle of confusion map

#define TG_SIZE		256

#define SCREEN_WIDTH	1024.f
#define SCREEN_HEIGHT	576.f

Texture2D gFrame : register(t0);
Texture2D gBlurred : register(t1);
Texture2D gCoC : register(t2);
RWTexture2D<float4> gOutput : register(u0);

[numthreads(TG_SIZE, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	float4 colour = gFrame[dispatchID.xy];
	float4 blurredColour = gBlurred[dispatchID.xy];

	float coc = gCoC[dispatchID.xy].r;

	gOutput[dispatchID.xy] = lerp(colour, blurredColour, coc);
}