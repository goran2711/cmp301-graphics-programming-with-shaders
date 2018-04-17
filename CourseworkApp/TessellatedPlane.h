// An overloaded PlaneMesh that uses patch topology instead of trianglelists

#pragma once
#include "..\DXFramework\PlaneMesh.h"

class TessellatedPlane : public PlaneMesh
{
public:
	TessellatedPlane(ID3D11Device* device, ID3D11DeviceContext* context, int resolution = 20);

	void sendData(ID3D11DeviceContext* context) override;
};

