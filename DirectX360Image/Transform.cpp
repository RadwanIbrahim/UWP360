#include "pch.h"
#include "Transform.h"
#include <WindowsNumerics.h>

using namespace DirectX360Image;

Transform::Transform() 
{
	Scale = float3::one();
	Translation = float3::zero();
	Rotation = float3::zero();
}

DirectX::XMMATRIX Transform::GetWorldMatrix()
{
	using namespace DirectX;

	return XMMatrixTranslation(0.0f, 0.0f, 0.0f)*
		XMMatrixScaling(Scale.x, Scale.y, Scale.z)*
		XMMatrixRotationX(XMConvertToRadians(Rotation.x))*XMMatrixRotationY(XMConvertToRadians(Rotation.y))*XMMatrixRotationZ(XMConvertToRadians(Rotation.z))*
		XMMatrixTranslation(Translation.x, Translation.y, Translation.z);
}