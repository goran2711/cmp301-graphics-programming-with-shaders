#include "blur_cs_header.hlsl"

// Horizontal gaussian blur pass
[numthreads(THREAD_GROUP_SIZE, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	HorizontalBlur(groupThreadID, dispatchID);
}
