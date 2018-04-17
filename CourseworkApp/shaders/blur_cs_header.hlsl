// Header file containing code for gaussian blur. Helps minimise duplicate code between horizontal and vertical shaders

#define THREAD_GROUP_SIZE	256
#define MAX_BLUR_RADIUS		32

cbuffer Settings : register(b0)
{
	static const int gBlurRadius = 5;
	static const float gWeights[11] =
	{
		0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f,
	};
};

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

groupshared float4 gCache[THREAD_GROUP_SIZE + 2 * gBlurRadius];

float4 GaussianBlur(int idx)
{
	float4 blurredColour = 0.f;
	for (int i = -gBlurRadius; i < gBlurRadius; ++i)
	{
		blurredColour += gCache[idx + gBlurRadius + i] * gWeights[i + gBlurRadius];
	}

	return blurredColour;
}

// Horizontal blur
void HorizontalBlur(int3 groupThreadID, int3 dispatchID)
{
	// Cache an extra pixel if this thread is one of the first
	if (groupThreadID.x < gBlurRadius)
	{
		int x = max(dispatchID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchID.y)];
	}

	// Cache an extra pixel if this thread is one of the last
	if (groupThreadID.x >= THREAD_GROUP_SIZE - gBlurRadius)
	{
		int x = min(dispatchID.x + gBlurRadius, gInput.Length.x - 1);
		gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchID.y)];
	}

	// Cache this thread's "1:1" pixel (in quotations because of offset)
	gCache[groupThreadID.x + gBlurRadius] = gInput[dispatchID.xy];

	// Wait for all the threads in the thread group to finish sampling
	GroupMemoryBarrierWithGroupSync();

	// Perform gaussian blur
	gOutput[dispatchID.xy] = GaussianBlur(groupThreadID.x);
}

// Vertical blur
void VerticalBlur(int3 groupThreadID, int3 dispatchID)
{
	// Cache an extra pixel if this thread is one of the first
	if (groupThreadID.y < gBlurRadius)
	{
		int y = max(dispatchID.y - gBlurRadius, 0);
		gCache[groupThreadID.y] = gInput[int2(dispatchID.x, y)];
	}

	// Cache an extra pixel if this thread is one of the last
	if (groupThreadID.y >= THREAD_GROUP_SIZE - gBlurRadius)
	{
		int y = min(dispatchID.y + gBlurRadius, gInput.Length.y - 1);
		gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchID.x, y)];
	}

	// Cache this thread's "1:1" pixel (in quotations because of offset)
	gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchID.xy, gInput.Length.xy - 1)];

	// Wait for all the threads in the thread group to finish sampling
	GroupMemoryBarrierWithGroupSync();

	// Do gaussian blur
	gOutput[dispatchID.xy] = GaussianBlur(groupThreadID.y);
}