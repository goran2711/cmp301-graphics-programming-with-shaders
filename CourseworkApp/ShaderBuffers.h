// A place where most application-side representation of contant buffers are kept to avoid code duplication

#pragma once
#include <DirectXMath.h>

namespace BufferType
{
	using namespace DirectX;

	__declspec(align(16))
	struct ShaderLight
	{
		XMFLOAT4 position;		// Offet:		 0
		XMFLOAT4 direction;		// Offet:		16
		XMFLOAT4 colour;		// Offset:		32

		float specularPower;	// Offset:		48
		float range;			// Offset:		52
		float spotAngle;		// Offset:		56
		float constAtt;			// Offset:		60

		float linearAtt;		// Offset:		64
		float quadraticAtt;		// Offset:		68
		int type;				// Offset:		72
		bool enabled;			// Offset:		76
		/////////////////////// // Total size:	80 bytes
	};

	template <int MAX_LIGHTS>
	/*__declspec(align(16))*/	// VS doesn't like aligning templates, but it's fine since XMFLOAT4 and ShaderLight are aligned anyway
	struct LightBufferType
	{
		XMFLOAT4 globalAmbient;
		ShaderLight lights[MAX_LIGHTS];
	};

	__declspec(align(16))
	struct FogBufferType
	{
		XMFLOAT4 fogColour;
		float fogMin;
		float fogRange;
	};

	__declspec(align(16))
	struct CameraBufferType
	{
		XMFLOAT3 position;
	};

	__declspec(align(16))
	struct MatrixBufferWithLightType
	{
		XMMATRIX world;
		XMMATRIX view;
		XMMATRIX projection;

		XMMATRIX shadowTransform;
	};

	__declspec(align(16))
	struct MatrixBufferWithoutWorldType
	{
		XMMATRIX view;
		XMMATRIX projection;
	};

	__declspec(align(16))
	struct MaterialPropertiesType
	{
		bool gUseHeightMap;
	};

	__declspec(align(16))
	struct TessellationPropertiesType
	{
		int gMinTess;
		int gMaxTess;
		float gMinDistance;
		float gMaxDistance;
	};
}