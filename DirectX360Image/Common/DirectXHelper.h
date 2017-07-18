#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d11_x.h>
#else
#include <d3d11_1.h>
#endif

#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
#if !defined(_XBOX_ONE) || !defined(_TITLE)
#pragma comment(lib,"dxguid.lib")
#endif
#endif

#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(x) IID_PPV_ARGS(x)
#endif

#include <exception>
#include <stdint.h>


#include <ppltasks.h>	// For create_task

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw Platform::Exception::CreateException(hr);
		}
	}

	// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
		auto path = (folder->Path + "\\" + ref new Platform::String(filename.c_str()));
		return create_task(Windows::Storage::StorageFile::GetFileFromPathAsync(path)).then([](StorageFile^ file)
			//return create_task(folder->GetFileAsync(Platform::StringReference(filename.c_str()))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}

	inline Concurrency::task<std::vector<byte>> ReadDataFromPackageAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;

		return create_task(Windows::Storage::StorageFile::GetFileFromApplicationUriAsync(ref new Windows::Foundation::Uri(ref new Platform::String(filename.c_str())))).then([](StorageFile^ file)
		{
			return FileIO::ReadBufferAsync(file);
		}).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});
	}


	inline Concurrency::task<std::vector<byte>> ReadDataFromHttpAsync(const std::wstring& filename)
	{
		using namespace Windows::Storage;
		using namespace Concurrency;
		using namespace Windows::Web::Http;


		HttpClient^ httpclient = ref new HttpClient();

		return create_task(httpclient->GetBufferAsync(ref new Windows::Foundation::Uri(ref new Platform::String(filename.data())))).then([](Streams::IBuffer^ fileBuffer) -> std::vector<byte>
		{
			std::vector<byte> returnBuffer;
			returnBuffer.resize(fileBuffer->Length);
			Streams::DataReader::FromBuffer(fileBuffer)->ReadBytes(Platform::ArrayReference<byte>(returnBuffer.data(), fileBuffer->Length));
			return returnBuffer;
		});

	}

	// Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
	inline float ConvertDipsToPixels(float dips, float dpi)
	{
		static const float dipsPerInch = 96.0f;
		return floorf(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
	}

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
		);

		return SUCCEEDED(hr);
	}
#endif


	class MapGuard : public D3D11_MAPPED_SUBRESOURCE
	{
	public:
		MapGuard(_In_ ID3D11DeviceContext* context,
			_In_ ID3D11Resource *resource,
			_In_ UINT subresource,
			_In_ D3D11_MAP mapType,
			_In_ UINT mapFlags)
			: mContext(context), mResource(resource), mSubresource(subresource)
		{
			HRESULT hr = mContext->Map(resource, subresource, mapType, mapFlags, this);
			if (FAILED(hr))
			{
				throw std::exception();
			}
		}

		~MapGuard()
		{
			mContext->Unmap(mResource, mSubresource);
		}

		uint8_t* get() const
		{
			return reinterpret_cast<uint8_t*>(pData);
		}
		uint8_t* get(size_t slice) const
		{
			return reinterpret_cast<uint8_t*>(pData) + (slice * DepthPitch);
		}

		uint8_t* scanline(size_t row) const
		{
			return reinterpret_cast<uint8_t*>(pData) + (row * RowPitch);
		}
		uint8_t* scanline(size_t slice, size_t row) const
		{
			return reinterpret_cast<uint8_t*>(pData) + (slice * DepthPitch) + (row * RowPitch);
		}

	private:
		ID3D11DeviceContext*    mContext;
		ID3D11Resource*         mResource;
		UINT                    mSubresource;

		MapGuard(MapGuard const&);
		MapGuard& operator= (MapGuard const&);
	};


	// Helper sets a D3D resource name string (used by PIX and debug layer leak reporting).
	template<UINT TNameLength>
	inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const char(&name)[TNameLength])
	{
#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
#if defined(_XBOX_ONE) && defined(_TITLE)
		wchar_t wname[MAX_PATH];
		int result = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name, TNameLength, wname, MAX_PATH);
		if (result > 0)
		{
			resource->SetName(wname);
		}
#else
		resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, name);
#endif
#else
		UNREFERENCED_PARAMETER(resource);
		UNREFERENCED_PARAMETER(name);
#endif
	}

	template<UINT TNameLength>
	inline void SetDebugObjectName(_In_ ID3D11DeviceChild* resource, _In_z_ const wchar_t(&name)[TNameLength])
	{
#if !defined(NO_D3D11_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
#if defined(_XBOX_ONE) && defined(_TITLE)
		resource->SetName(name);
#else
		char aname[MAX_PATH];
		int result = WideCharToMultiByte(CP_ACP, 0, name, TNameLength, aname, MAX_PATH, nullptr, nullptr);
		if (result > 0)
		{
			resource->SetPrivateData(WKPDID_D3DDebugObjectName, TNameLength - 1, aname);
		}
#endif
#else
		UNREFERENCED_PARAMETER(resource);
		UNREFERENCED_PARAMETER(name);
#endif
	}

}
