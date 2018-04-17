#include "TessellatedPlane.h"


TessellatedPlane::TessellatedPlane(ID3D11Device * device, ID3D11DeviceContext * context, int resolution)
	:	PlaneMesh(device, context, resolution)
{
}

void TessellatedPlane::sendData(ID3D11DeviceContext * context)
{
	static constexpr UINT stride[1] = { sizeof(VertexType) };
	static constexpr UINT offset[1] = { 0U };

	context->IASetVertexBuffers(0, 1, &vertexBuffer, stride, offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
}
