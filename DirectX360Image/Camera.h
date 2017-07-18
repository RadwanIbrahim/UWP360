#pragma once
#include <WindowsNumerics.h>
#include <pch.h>
using namespace DirectX;
using namespace Windows::Foundation::Numerics;

namespace DirectX360Image
{
	public ref class Camera sealed
	{
	public:
		
		//void Update();
		void SetPosition(float x, float y, float z);
		// Get frustum properties.
		property float NearZ {float get() { return mNearZ; } }
		property float FarZ {float get() { return mFarZ; }}
		property float Aspect {float get() { return mAspect; } }
		property float FovY {float get() { return mFovY; }}
		property float FovX {float get() {
			float halfWidth = 0.5f*NearWindowWidth;
			return 2.0f*atan(halfWidth / mNearZ);
		}}
		// Get near and far plane dimensions in view space coordinates.
		property float NearWindowWidth {float get() { return mAspect * mNearWindowHeight; }}
		property float NearWindowHeight {float get() { return mNearWindowHeight; }}
		property float FarWindowWidth {float get() { return mAspect * mFarWindowHeight; }}
		property float FarWindowHeight {float get() {return mFarWindowHeight; }}
		// Strafe/Walk the camera a distance d.
		void Strafe(float d);
		void Walk(float d);

		// Rotate the camera.
		void Pitch(float angle);
		void RotateY(float angle);

		void SetLens(float fovY, float aspect, float4x4 orientation, float zn, float zf);
		void LookAt(const float3& pos, const float3& target, const float3& up);

	internal:
		Camera();
		// Get/Set world camera position.
		XMVECTOR GetPositionXM()const;
		XMFLOAT3 GetPosition()const;

		void SetPosition(const XMFLOAT3& v);

		// Get camera basis vectors.
		XMVECTOR GetRightXM()const;
		XMFLOAT3 GetRight()const;
		XMVECTOR GetUpXM()const;
		XMFLOAT3 GetUp()const;
		XMVECTOR GetLookXM()const;
		XMFLOAT3 GetLook()const;

		// Set frustum.

		void SetOrientationMatrix(XMFLOAT4X4 orientation);

		// Define camera space via LookAt parameters.
		void LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);


		// Get View/Proj matrices.
		XMMATRIX View()const;
		XMMATRIX Proj()const;
		XMMATRIX ViewProj()const;


		// After modifying camera position/orientation, call to rebuild the view matrix.
		void UpdateViewMatrix();

	private:
		~Camera();
		// Camera coordinate system with coordinates relative to world space.
		XMFLOAT3 mPosition;
		XMFLOAT3 mRight;
		XMFLOAT3 mUp;
		XMFLOAT3 mLook;

		// Cache frustum properties.
		float mNearZ;
		float mFarZ;
		float mAspect;
		float mFovY;
		float mNearWindowHeight;
		float mFarWindowHeight;

		// Cache View/Proj matrices.
		XMFLOAT4X4 mView;
		XMFLOAT4X4 mProj;
		float4x4 mOrientation;
	};

}