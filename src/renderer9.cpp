#include "scene.h"
#include "util.h"

#include <d3d9.h>
#include <vector>

using namespace std;

namespace {

	class Texture2D : public ISurface
	{
	private:
		std::shared_ptr<IDirect3DDevice9Ex> const device_;
		std::shared_ptr<IDirect3DTexture9> const texture_;
		void* share_handle_;
		uint32_t width_;
		uint32_t height_;

	public:
		Texture2D(
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

		void* share_handle() const override {
			return share_handle_; 
		}
		
		uint32_t width() const override { 
			return width_; 
		}
		
		uint32_t height() const override { 
			return height_; 
		}

		operator IDirect3DTexture9*() {
			return texture_.get();
		}
	};
	
	//
	// manages offscreen render-target(s)
	//
	class FrameBuffer
	{
	private:
		std::shared_ptr<IDirect3DDevice9Ex> const device_;
		std::shared_ptr<IDirect3DSurface9> saved_target_;
		std::vector<std::shared_ptr<Texture2D>> buffers_;

	public:

		FrameBuffer(
			shared_ptr<IDirect3DDevice9Ex> const device,
			vector<shared_ptr<Texture2D>> const& buffers)
			: device_(device)
		{
			buffers_.assign(buffers.begin(), buffers.end());
		}

		void bind(void* target)
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
		
		void unbind()
		{
			// restore original render-target
			if (saved_target_)
			{
				device_->SetRenderTarget(0, saved_target_.get());
				update_viewport(saved_target_.get());
				saved_target_.reset();
			}
		}

		size_t buffer_count() const {
			return buffers_.size();
		}

		std::shared_ptr<Texture2D> buffer(size_t n) const
		{
			if (n < buffers_.size()) {
				return buffers_[n];
			}
			return nullptr;
		}

	private:

		void update_viewport(IDirect3DSurface9* surface)
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
	};
	

	class Renderer : public IScene
	{
	private:
		shared_ptr<IDirect3DDevice9Ex> const device_;
		shared_ptr<FrameBuffer> const frame_buffer_;
		shared_ptr<ISurfaceQueue> const queue_;
		shared_ptr<IDirect3DQuery9> flush_query_;

	public:
		Renderer(shared_ptr<IDirect3DDevice9Ex> const& device,
			shared_ptr<FrameBuffer> const& frame_buffer,
			shared_ptr<ISurfaceQueue> const& queue)
			: device_(device)
			, frame_buffer_(frame_buffer)
			, queue_(queue)
		{
		}

		string gpu() const override 
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

		void tick(double) override
		{
		}

		void clear(float red, float green, float blue, float alpha)
		{
			auto const color = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
			device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
		}

		void render() override
		{
			auto const target = queue_->checkout(100);
			if (target)
			{
				clear(0.0f, 0.0f, 0.0f, 0.0f);

				device_->BeginScene();

				{
					frame_buffer_->bind(target->share_handle());
					
					clear(0.0f, 0.0f, 1.0f, 1.0f);

					frame_buffer_->unbind();
				}

				// ensure the D3D9 device is done writing to our texture buffer
				flush();

				// place on queue so a producer will be notified
				queue_->produce(target);
			}

			device_->EndScene();
		}

		void present(int32_t) override
		{
			device_->Present(nullptr, nullptr, nullptr, nullptr);
		}

		shared_ptr<ISurfaceQueue> queue() const {
			return queue_;
		}
		
		bool flush()
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

	};
}


shared_ptr<IDirect3DDevice9Ex> create_device(HWND window)
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
		return to_com_ptr(device);
	}

	return nullptr;
}

shared_ptr<FrameBuffer> create_frame_buffer(
		shared_ptr<IDirect3DDevice9Ex> const& device,
		uint32_t buffers, 
		uint32_t width, 
		uint32_t height)
{
	vector<std::shared_ptr<Texture2D>> textures;

	for (size_t n = 0; n < buffers; n++)
	{
		HANDLE share = nullptr;
		IDirect3DTexture9* texture = nullptr;
		auto const hr = device->CreateTexture(
			width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, &share);
		if (SUCCEEDED(hr)) {
			textures.push_back(make_shared<Texture2D>(device, texture, share));
		}
	}

	if (!textures.empty()) {
		return make_shared<FrameBuffer>(device, textures);
	}
	return nullptr;
}

shared_ptr<IScene> create_producer(void* native_window)
{
	auto const dev = create_device((HWND)native_window);
	if (!dev) {
		return nullptr;
	}

	RECT rc;
	GetClientRect((HWND)native_window, &rc);

	auto const w = rc.right - rc.left;
	auto const h= rc.bottom - rc.top;

	// create shared buffers for delivery to a consumer
	auto const swapchain = create_frame_buffer(dev, 3, w, h);
	if (!swapchain) {
		return nullptr;
	}
	
	// notify the surface queue about the shared textures 
	// we will be rendering to
	auto const queue = create_surface_queue();
	for (size_t n = 0; n < swapchain->buffer_count(); ++n) {
		queue->checkin(swapchain->buffer(n));			
	}
	
	auto const producer = make_shared<Renderer>(dev, swapchain, queue);
	
	string title("Direct3D9 Producer");
	title.append(" - [gpu: ");
	title.append(producer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return producer;
}


