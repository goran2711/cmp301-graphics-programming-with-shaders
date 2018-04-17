// Circle of confusion shader
// Calculates the depth difference between the current pixel and the center pixel and stores it in a texture
// which will later be sampled when merging the blurred and unblurred frame buffer (for depth of field effect)

#define TG_SIZE		256

#define SCREEN_WIDTH	1024.f
#define SCREEN_HEIGHT	576.f

Texture2D gDepth : register(t0);

// Rather wasteful to use a R8G8B8A8 texture for this, but it's what RenderTexture creates
RWTexture2D<float4> gCoC : register(u0);

[numthreads(TG_SIZE, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	float pixelDepth = gDepth[dispatchID.xy].r;
	float centreDepth = gDepth[int2(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2)].r;

	// How far ahead/behind the centre fragment this fragment is
	float delta = abs(centreDepth - pixelDepth);

	// Amplify it a bit
	delta *= 7.5f;

	gCoC[dispatchID.xy] = saturate(float4(delta, delta, delta, 1.f));
}