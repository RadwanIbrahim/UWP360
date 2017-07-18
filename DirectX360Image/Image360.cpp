#pragma once
#include "pch.h"
#include "Image360.h"
#include "Common\DirectXHelper.h"
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <math.h>
#include <ppltasks.h>
#include <windows.ui.xaml.media.dxinterop.h>
#include "Camera.h"
#include "Transform.h"
#include <math.h> 

using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Concurrency;
using namespace DirectX;
using namespace D2D1;
using namespace DirectX360Image;
using namespace VSD3DStarter;
using namespace DX;



Image360::Image360()
{
	critical_section::scoped_lock lock(m_criticalSection);
	m_Transform = ref new DirectX360Image::Transform();
	m_Transform->Scale = float3(5, 5, 5);
	m_Camera = ref new DirectX360Image::Camera();
	m_Camera->LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 4.0f), float3(0.0f, 1.0f, 0.0f));
	this->ManipulationMode = ManipulationModes::TranslateX| ManipulationModes::TranslateY;
	this->Unloaded += ref new RoutedEventHandler(this, &Image360::OnUnloaded);
	this->ManipulationDelta += ref new Windows::UI::Xaml::Input::ManipulationDeltaEventHandler(this, &Image360::OnManipulationDelta);
	CreateDeviceIndependentResources();
	CreateDeviceResources();
	CreateSizeDependentResources();

}

Image360::~Image360()
{
	m_renderLoopWorker->Cancel();
	for (auto m : m_meshModels)
		delete m;
	m_meshModels.clear();

	if (m_skinnedMeshRenderer != nullptr)
		delete	m_skinnedMeshRenderer;
}

DependencyProperty^ DirectX360Image::Image360::m_TextureSourceProperty = DependencyProperty::Register("TextureSource", Windows::Foundation::Uri::typeid, Image360::typeid, ref new PropertyMetadata(nullptr,
	ref new PropertyChangedCallback(&Image360::OnTextureSourceChanged))
);

void Image360::OnTextureSourceChanged(DependencyObject^ d, DependencyPropertyChangedEventArgs^ e)
{
	Image360^ That = (Image360^)d;
	Windows::Foundation::Uri^ source = (Windows::Foundation::Uri^)(e->NewValue);
	That->SetTextureSource(source);
}

Platform::String^ Image360::ModelName::get()
{
	return m_ModelName;
}


void Image360::ModelName::set(Platform::String^ value)
{
	m_ModelName = value;
}


DirectX360Image::Transform^ Image360::Transform::get()
{
	return m_Transform;
}

DirectX360Image::Camera^ Image360::Camera::get()
{
	return m_Camera;
}


void DirectX360Image::Image360::SetTextureSource(Windows::Foundation::Uri ^source)
{
	if (m_meshModels.size() == 0)
	{
		auto CreateModelTask = LoadAsync(m_graphics, ModelName->Data(), TextureSource->RawUri->Data());

		CreateModelTask.then([this]() {
			m_loadingComplete = true;
			ModelLoaded(this, true);
		});
	}
	else
	{

		m_graphics.GetOrCreateTextureAsync(source->RawUri->Data()).then([this](ID3D11ShaderResourceView* textureResource)
		{
			m_Camera->LookAt(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 4.0f), float3(0.0f, 1.0f, 0.0f));
			this->m_meshModels[0]->Materials()[0].Textures[0] = textureResource;
		});

	}

	StartRenderLoop();
}

void Image360::OnUnloaded(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^e)
{
	StopRenderLoop();
}

void Image360::OnManipulationDelta(Platform::Object ^sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs ^e)
{	
	m_Camera->RotateY(XMConvertToRadians(-e->Delta.Translation.X));
	m_Camera->Pitch(XMConvertToRadians(-e->Delta.Translation.Y));
}

concurrency::task<void> Image360::LoadAsync(Graphics& graphics,
	const std::wstring& meshFilename,
	const std::wstring& TextureUrl,
	const bool IsAnimated,
	const std::wstring& shaderPathLocation,
	const std::wstring& texturePathLocation
)

{
	isanimated = IsAnimated;

	auto task = Mesh::LoadFromFileAsync(graphics, L"Sphere.cmo", TextureUrl, shaderPathLocation, texturePathLocation, m_meshModels);

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

void Image360::StartRenderLoop()
{
	// If the animation render loop is already running then do not start another thread.
	if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
	{
		return;
	}

	// Create a task that will be run on a background thread.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// Calculate the updated frame and render once per vertical blanking interval.
		while (action->Status == AsyncStatus::Started)
		{
			m_timer.Tick([&]()
			{
				critical_section::scoped_lock lock(m_criticalSection);
				Render();
			});

			// Halt the thread until the next vblank is reached.
			// This ensures the app isn't updating and rendering faster than the display can refresh,
			// which would unnecessarily spend extra CPU and GPU resources.  This helps improve battery life.
			m_dxgiOutput->WaitForVBlank();
		}
	});

	// Run task on a dedicated high priority background thread.
	m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

void Image360::StopRenderLoop()
{
	// Cancel the asynchronous task and let the render thread exit.
	m_renderLoopWorker->Cancel();
}

void Image360::Render()
{
	if (!m_loadingComplete || m_timer.GetFrameCount() == 0)
	{
		return;
	}
	// Set render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_renderTargetView.Get() };
	m_d3dContext->OMSetRenderTargets(1, targets, m_depthStencilView.Get());
	XMVECTORF32 Transparent = { 0.000000000f, 1.000000000f, 1.000000000f, 1.000000000f };
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), DirectX::Colors::Transparent);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_TransformMatrix = this->Transform->GetWorldMatrix();

	if (isanimated)
	{

		for (UINT i = 0; i < m_meshModels.size(); i++)
		{

			if (m_meshModels[i]->Tag != nullptr)
			{
				// Mesh has animation - render skinned mesh.
				m_skinnedMeshRenderer->RenderSkinnedMesh(m_meshModels[i], m_graphics, m_Camera, m_TransformMatrix);
			}
			else
			{
				// Mesh does not have animation - render as usual.
				m_meshModels[i]->Render(m_graphics, m_Camera, m_TransformMatrix);
			}
		}
	}
	else
	{
		for (UINT i = 0; i < m_meshModels.size(); i++)
		{
			// Mesh does not have animation - render as usual.
			m_meshModels[i]->Render(m_graphics, m_Camera, m_TransformMatrix);

		}
	}


	Present();
}

void Image360::CreateDeviceResources()
{
	DirectXPanelBase::CreateDeviceResources();

	ComPtr<IDXGIFactory1> dxgiFactory;
	ThrowIfFailed(
		CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
	);

	ComPtr<IDXGIAdapter> dxgiAdapter;
	ThrowIfFailed(
		dxgiFactory->EnumAdapters(0, &dxgiAdapter)
	);

	ThrowIfFailed(
		dxgiAdapter->EnumOutputs(0, &m_dxgiOutput)
	);

	m_graphics.Initialize(m_d3dDevice.Get(), m_d3dContext.Get(), m_d3dFeatureLevel);


	CD3D11_RASTERIZER_DESC d3dRas(D3D11_DEFAULT);
	d3dRas.CullMode = D3D11_CULL_NONE;
	d3dRas.MultisampleEnable = true;
	d3dRas.AntialiasedLineEnable = true;

	ComPtr<ID3D11RasterizerState> p3d3RasState;
	m_d3dDevice->CreateRasterizerState(&d3dRas, &p3d3RasState);
	m_d3dContext->RSSetState(p3d3RasState.Get());


}

void Image360::CreateSizeDependentResources()
{

	m_renderTargetView = nullptr;
	m_depthStencilView = nullptr;

	DirectXPanelBase::CreateSizeDependentResources();

	ComPtr<ID3D11Texture2D> backBuffer;
	ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
	);

	// Create render target view.
	ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&m_renderTargetView)
	);

	// Create and set viewport.
	D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(
		0.0f,
		0.0f,
		m_renderTargetWidth,
		m_renderTargetHeight
	);

	m_d3dContext->RSSetViewports(1, &viewport);

	// Create depth/stencil buffer descriptor.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		static_cast<UINT>(m_renderTargetWidth),
		static_cast<UINT>(m_renderTargetHeight),
		1,
		1,
		D3D11_BIND_DEPTH_STENCIL
	);

	// Allocate a 2-D surface as the depth/stencil buffer.
	ComPtr<ID3D11Texture2D> depthStencil;
	ThrowIfFailed(
		m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencil)
	);

	// Create depth/stencil view based on depth/stencil buffer.
	const CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
	ThrowIfFailed(
		m_d3dDevice->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_depthStencilView
		)
	);

	m_miscConstants.ViewportHeight = m_renderTargetHeight;
	m_miscConstants.ViewportWidth = m_renderTargetWidth;
	m_graphics.UpdateMiscConstants(m_miscConstants);


	float aspectRatio = m_renderTargetWidth / m_renderTargetHeight;
	float fovAngleY = 70.0f * XM_PI / 180.0f;


	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);


	float4x4 orientation =
		float4x4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);

	m_Camera->SetLens(fovAngleY, aspectRatio, orientation, 1.00f, 1000.0f);
	m_Camera->UpdateViewMatrix();


	// Setup lightinUpdateViewMatrix();g for our scene.
	static const XMVECTORF32 s_vPos = { 0.0f, 1.0f, 0.0f, 0.f };

	XMFLOAT4 dir;
	DirectX::XMStoreFloat4(&dir, XMVector3Normalize(s_vPos));

	m_lightConstants.ActiveLights = 3;
	m_lightConstants.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_lightConstants.IsPointLight[0] = true;
	m_lightConstants.LightColor[0] = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_lightConstants.LightDirection[0] = dir;
	m_lightConstants.LightSpecularIntensity[0].x = 2;

	m_graphics.UpdateLightConstants(m_lightConstants);
}


void Image360::Clear(Color color)
{
	// Convert color values.
	const float clearColor[4] = { color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f };
	// Clear render target view with given color.
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
}
