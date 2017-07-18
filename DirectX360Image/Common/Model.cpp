#include "pch.h"
#include "Model.h"
#include "ppltasks_extra.h"
#include "Common\Camera.h"
#include"Animation\Animation.h"
#include "VSD3DStarter.h"

using namespace DirectX;
using namespace DX;




concurrency::task<void> Model::LoadAsync(Graphics& graphics,
	const std::wstring& meshFilename,
	const std::wstring& TextureUrl,
	const bool IsAnimated,
	const std::wstring& shaderPathLocation,
	const std::wstring& texturePathLocation
)

{
	isanimated = IsAnimated;
	
	auto task = Mesh::LoadFromFileAsync(graphics, L"Sphere.cmo",TextureUrl, shaderPathLocation, texturePathLocation, m_meshModels);

	if (isanimated)
	{

		return task.then([this]()
		{
			for (Mesh* m : m_meshModels)
			{
				if (m->BoneInfoCollection().empty() == false)
				{
					auto animState = new AnimationState();
					animState->m_boneWorldTransforms.resize(m->BoneInfoCollection().size());
					m->Tag = animState;
				}
			}

		}).then([this, &graphics]() {
			m_skinnedMeshRenderer = new SkinnedMeshRenderer();
			return m_skinnedMeshRenderer->InitializeAsync(graphics.GetDevice(), graphics.GetDeviceContext());
		});
	}
	else
	{
		return task;
	}


}

void Model::Render(const Graphics& graphics, const DX::Camera& camera, DirectX::XMMATRIX* WorldMatrix)
{

	if (isanimated)
	{

		for (UINT i = 0; i < m_meshModels.size(); i++)
		{

			if (m_meshModels[i]->Tag != nullptr)
			{
				// Mesh has animation - render skinned mesh.
				m_skinnedMeshRenderer->RenderSkinnedMesh(m_meshModels[i], graphics, camera, WorldMatrix);
			}
			else
			{
				// Mesh does not have animation - render as usual.
				m_meshModels[i]->Render(graphics, camera, WorldMatrix);
			}
		}
	}
	else
	{
		for (UINT i = 0; i < m_meshModels.size(); i++)
		{
			// Mesh does not have animation - render as usual.
			m_meshModels[i]->Render(graphics, camera, WorldMatrix);

		}
	}

}

void Model::UpdateAnimation(float TimeDelta)
{
	if (isanimated)
		m_skinnedMeshRenderer->UpdateAnimation(TimeDelta, m_meshModels);
}



float Model::GetX()const { return x; }
float Model::GetY()const { return y; }
float Model::GetZ()const { return z; }

void Model::TranslateX(const float& x) {
	for (auto m : m_meshModels)
	{
		m->TranslateX(x);
	}
	this->x = x;
}
void Model::TranslateY(const float& y)
{
	this->y = y;
	for (auto m : m_meshModels)
	{
		m->TranslateY(y);
	}
}
void Model::TranslateZ(const float& z)
{
	this->z = z;
	for (auto m : m_meshModels)
	{
		m->TranslateZ(z);
	}
}

float Model::GetScaleX()const { return scalex; }
float Model::GetScaleY()const { return scaley; }
float Model::GetScaleZ()const { return scalez; }

void Model::ScaleX(const float& x)
{
	this->scalex = x;
	for (auto m : m_meshModels)
	{
		m->ScaleX(x);
	}
}
void Model::ScaleY(const float& y)
{
	this->scaley = y;
	for (auto m : m_meshModels)
	{
		m->ScaleY(y);
	}
}
void Model::ScaleZ(const float& z)
{
	this->scalez = z;
	for (auto m : m_meshModels)
	{
		m->ScaleZ(z);
	}
}

float Model::GetRotateX()const { return rotatex; }
float Model::GetRotateY()const { return rotatey; }
float Model::GetRotateZ()const { return rotatez; }

void Model::RotateX(const float& x)
{
	this->rotatex = x;
	for (auto m : m_meshModels)
	{
		m->RotateX(x);
	}
}
void Model::RotateY(const float& y)
{
	this->rotatey = y;
	for (auto m : m_meshModels)
	{
		m->RotateY(y);
	}
}
void Model::RotateZ(const float& z)
{
	this->rotatez = z;
	for (auto m : m_meshModels)
	{
		m->RotateZ(z);
	}
}

void Model::Rotate(const float&x, const float&y, const float& z)
{
	rotatex = x;
	rotatey = y;
	rotatez = z;
	for (auto m : m_meshModels)
	{
		m->RotateX(x);
		m->RotateY(y);
		m->RotateZ(z);
	}
}
void Model::Translate(const float&tx, const float&ty, const float& tz)
{
	x = tx;
	y = ty;
	z = tz;
	for (auto m : m_meshModels)
	{
		m->TranslateX(tx);
		m->TranslateY(ty);
		m->TranslateZ(tz);
	}
}
void Model::Scale(const float&x, const float&y, const float& z)
{
	scalex = x;
	scaley = y;
	scalez = z;
	for (auto m : m_meshModels)
	{
		m->ScaleX(x);
		m->ScaleY(y);
		m->ScaleZ(z);
	}
}


void Model::TranslateSubModel(const std::wstring& Name, const float& x, const float& y, const float& z)
{
	auto model = GetSubModel(Name);
	if (model != nullptr)
	{

		model->TranslateX(x);
		model->TranslateY(y);
		model->TranslateZ(z);
	}
	else
	{
		throw std::exception("there is no Model with this name");
	}

}
void Model::ScaleSubModel(const std::wstring& Name, const float& x, const float& y, const float& z)
{
	auto model = GetSubModel(Name);
	if (model != nullptr)
	{
		model->ScaleX(x);
		model->ScaleY(y);
		model->ScaleZ(z);
	}
	else
	{
		throw std::exception("there is no Model with this name");
	}

}
void Model::RotateSubModel(const std::wstring& Name, const float& x, const float& y, const float& z)
{
	auto model = GetSubModel(Name);
	if (model != nullptr)
	{
		model->RotateX(x);
		model->RotateY(y);
		model->RotateZ(z);
	}
	else
	{
		throw std::exception("there is no Model with this name");
	}

}

Mesh* Model::GetSubModel(const std::wstring& Name)
{
	for (Mesh* m : m_meshModels)
	{
		if (Name == m->Name())
			return m;
	}
	return nullptr;
}


Model::~Model()
{

	for (Mesh* m : m_meshModels)
	{
		if (m != nullptr)
		{
			AnimationState* animState = (AnimationState*)m->Tag;
			if (animState != nullptr)
			{
				m->Tag = nullptr;
				delete animState;
			}
		}
		delete m;
	}
	delete m_skinnedMeshRenderer;
	m_meshModels.clear();


}

bool Model::IsColideWith(Model* Model, VSD3DStarter::CollisionTypes CollisionType)
{
	for (auto thissubmodel : m_meshModels)
	{
		for (auto submodel : Model->m_meshModels)
		{
			if (thissubmodel->IsColideWith(submodel))
				return true;
		}
	}
	return false;
}