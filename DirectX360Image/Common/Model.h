
#pragma once

#include <vector>
#include "ppltasks_extra.h"
#include "Camera.h"
#include"Animation\Animation.h"
#include "VSD3DStarter.h"

using namespace  VSD3DStarter;
namespace DX
{
	class Model
	{

	public:

		concurrency::task<void> LoadAsync(Graphics& graphics,
			const std::wstring& meshFilename,
			const std::wstring& TextureUrl,
			const bool IsAnimated = false,
			const std::wstring& shaderPathLocation = L"",
			const std::wstring& texturePathLocation = L""
			);

		void Render(const Graphics& graphics, const DX::Camera& camera, DirectX::XMMATRIX* WorldMatrix = nullptr);
		void UpdateAnimation(float TimeDelta);

		bool IsColideWith(Model* Model, VSD3DStarter::CollisionTypes CollisionType = VSD3DStarter::CollisionTypes::BoundingSphere);

		float GetX()const;
		float GetY()const;
		float GetZ()const;

		float GetScaleX()const;
		float GetScaleY()const;
		float GetScaleZ()const;

		float GetRotateX()const;
		float GetRotateY()const;
		float GetRotateZ()const;

		void TranslateX(const float& x);
		void TranslateY(const float& y);
		void TranslateZ(const float& z);

		void ScaleX(const float& x);
		void ScaleY(const float& y);
		void ScaleZ(const float& z);

		void RotateX(const float& x);
		void RotateY(const float& y);
		void RotateZ(const float& z);

		void Rotate(const float& x, const float& y, const float& z);
		void Translate(const float& tx, const float& ty, const float& tz);
		void Scale(const float& x, const float& y, const float& z);

		void TranslateSubModel(const std::wstring& Name, const float& x, const float& y, const float& z);
		void ScaleSubModel(const std::wstring& Name, const float& x, const float& y, const float& z);
		void RotateSubModel(const std::wstring& Name, const float& x, const float& y, const float& z);

		~Model();

	private:
		Mesh* GetSubModel(const std::wstring& Name);

	public:
		std::vector<VSD3DStarter::Mesh*> m_meshModels;


	private:

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		float scalex = 1.0f;
		float scaley = 1.0f;
		float scalez = 1.0f;

		float rotatex = 0.0f;
		float rotatey = 0.0f;
		float rotatez = 0.0f;
		SkinnedMeshRenderer* m_skinnedMeshRenderer;
		bool isanimated;

	};
}
