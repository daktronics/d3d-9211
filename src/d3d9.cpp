#include "d3d9.h"
#include "util.h"

#include <d3dcompiler.h>
#include <directxmath.h>

using namespace std;

namespace d3d9 {

	Device::Device(IDirect3DDevice9Ex* pdev)
		: device_(to_com_ptr(pdev))
	{
_lib_compiler = LoadLibrary(L"d3dcompiler_47.dll");
	}

	string Device::adapter_name() const
	{
		D3DDEVICE_CREATION_PARAMETERS params = {};
		device_->GetCreationParameters(&params);
		IDirect3D9* parent = nullptr;
		device_->GetDirect3D(&parent);
		if (parent)
		{
			D3DADAPTER_IDENTIFIER9 id;
			parent->GetAdapterIdentifier(params.AdapterOrdinal, 0, &id);
			parent->Release();
			return id.Description;
		}
		return "n/a";
	}

	shared_ptr<SwapChain> Device::create_swapchain(
		uint32_t buffers, uint32_t width, uint32_t height)
	{
		vector<std::shared_ptr<Texture2D>> textures;

		for (size_t n = 0; n < buffers; n++)
		{
			HANDLE share = nullptr;
			IDirect3DTexture9* texture = nullptr;
			auto const hr = device_->CreateTexture(
				width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, &share);
			if (SUCCEEDED(hr)) {
				textures.push_back(make_shared<Texture2D>(device_, texture, share));
			}
		}

		if (!textures.empty()) {
			return make_shared<SwapChain>(device_, textures);
		}
		return nullptr;
	}

	shared_ptr<Texture2D> Device::create_shared_texture(uint32_t width, uint32_t height)
	{
		HANDLE share = nullptr;
		IDirect3DTexture9* texture = nullptr;
		auto const hr = device_->CreateTexture(
			width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, &share);
		if (SUCCEEDED(hr)) {
			return make_shared<Texture2D>(device_, texture, share);
		}
		return nullptr;
	}

	void Device::clear(float red, float green, float blue, float alpha)
	{
		auto const color = D3DCOLOR_COLORVALUE(red, green, blue, alpha);

		device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
	}

	void Device::begin_scene()
	{
		device_->BeginScene();
	}

	void Device::end_scene()
	{
		device_->EndScene();
	}

	void Device::present()
	{
		device_->Present(nullptr, nullptr, nullptr, nullptr);
	}

	bool Device::flush()
	{
		if (!flush_query_)
		{
			IDirect3DQuery9* query = nullptr;
			auto const hr = device_->CreateQuery(D3DQUERYTYPE_EVENT, &query);
			if (SUCCEEDED(hr)) {
				flush_query_ = to_com_ptr<>(query);
			}
		}

		if (!flush_query_) {
			return false;
		}		
		if (FAILED(flush_query_->Issue(D3DISSUE_END))) {
			return false;
		}

		auto const start = time_now();
		uint32_t count = 1;
		while (flush_query_->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
		{
			if ((count % 2) == 0) {
				Sleep(0);
			}
			++count;

			// no way this should take more than 1 second
			if ((time_now() - start) > 1000000) 
			{
				log_message("timeout waiting for D3D9 flush\n");
				return false;
			}
		}
		return true;
	}


	Texture2D::Texture2D(
		shared_ptr<IDirect3DDevice9Ex> const device,
		IDirect3DTexture9* texture, 
		void* share_handle)
		: device_(device)
		, texture_(texture)
		, share_handle_(share_handle)
	{
		D3DSURFACE_DESC desc;
		texture_->GetLevelDesc(0, &desc);
		width_ = desc.Width;
		height_ = desc.Height;
	}

	void* Texture2D::share_handle() const {
		return share_handle_;
	}

	uint32_t Texture2D::width() const {
		return width_;
	}

	uint32_t Texture2D::height() const {
		return height_;
	}


	SwapChain::SwapChain(
		shared_ptr<IDirect3DDevice9Ex> const device,
		vector<std::shared_ptr<Texture2D>> const& buffers)
		: device_(device)
	{
		buffers_.assign(buffers.begin(), buffers.end());
	}

	void SwapChain::bind(void* target)
	{
		// save original render-target
		IDirect3DSurface9* rt = nullptr;
		device_->GetRenderTarget(0, &rt);
		saved_target_ = to_com_ptr<>(rt);

		shared_ptr<Texture2D> texture;

		for (auto const& b : buffers_)
		{
			if (b->share_handle() == target) {
				texture = b;
				break;
			}		
		}

		if (texture)
		{
			auto const tex = (IDirect3DTexture9*)(*texture);
			if (tex)
			{
				IDirect3DSurface9* surf = nullptr;
				tex->GetSurfaceLevel(0, &surf);
				if (surf)
				{
					device_->SetRenderTarget(0, surf);
					update_viewport(surf);
					surf->Release();
				}
			}
		}
	}

	void SwapChain::unbind()
	{
		// restore original render-target
		if (saved_target_)
		{
			device_->SetRenderTarget(0, saved_target_.get());
			update_viewport(saved_target_.get());
			saved_target_.reset();
		}
	}

	void SwapChain::update_viewport(IDirect3DSurface9* surface)
	{
		if (surface)
		{
			D3DSURFACE_DESC desc;
			surface->GetDesc(&desc);

			D3DVIEWPORT9 vp;
			vp.X = 0;
			vp.Y = 0;
			vp.Width = desc.Width;
			vp.Height = desc.Height;
			vp.MinZ = 0.0f;
			vp.MaxZ = 1.0f;

			device_->SetViewport(&vp);
		}
	}


	shared_ptr<Device> create_device(HWND window)
	{
		IDirect3D9Ex* d3d9 = nullptr;
		auto hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9);
		if (FAILED(hr)) {
			return nullptr;
		}

		D3DPRESENT_PARAMETERS pp = {};
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		pp.hDeviceWindow = window;
		pp.Windowed = TRUE;
		
		IDirect3DDevice9Ex* device = nullptr;
		hr = d3d9->CreateDeviceEx(
			0,
			D3DDEVTYPE_HAL,
			window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&pp,
			nullptr,
			&device);	
		d3d9->Release();		
		
		if (SUCCEEDED(hr)) {
			return make_shared<Device>(device);
		}		
		
		return nullptr;
	}
}