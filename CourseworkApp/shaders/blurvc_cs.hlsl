#include "blur_cs_header.hlsl"

// Vertical gaussian blur pass
[numthreads(1, THREAD_GROUP_SIZE, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchID : SV_DispatchThreadID)
{
	VerticalBlur(groupThreadID, dispatchID);
}
