#include "scene.h"
#include "util.h"

#include <d3d9.h>

#include <vector>
#include <math.h>

using namespace std;

namespace {

	struct VERTEX
	{
		float x;
		float y;
		float z;
		DWORD color;
		float u;
		float v;
	};

	//
	// simply because we're not messing with D3D9X
	//
	void matrix_identity(D3DMATRIX& dst)
	{
		dst._11 = 1.0f; dst._12 = 0.0f; dst._13 = 0.0f; dst._14 = 0.0f;
		dst._21 = 0.0f; dst._22 = 1.0f; dst._23 = 0.0f; dst._24 = 0.0f;
		dst._31 = 0.0f; dst._32 = 0.0f; dst._33 = 1.0f; dst._34 = 0.0f;
		dst._41 = 0.0f; dst._42 = 0.0f; dst._43 = 0.0f; dst._44 = 1.0f;
	}

	void matrix_multiply(D3DMATRIX& dst, D3DMATRIX const& m1, D3DMATRIX const& m2)
	{
		D3DMATRIX out;
		for (int32_t i = 0; i < 4; ++i)
		{
			for (int32_t j = 0; j < 4; ++j)
			{
				out.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] *
					m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
			}
		}
		dst = out;
	}

	void matrix_translation(D3DMATRIX& dst, float x, float y, float z)
	{
		matrix_identity(dst);
		dst.m[3][0] = x;
		dst.m[3][1] = y;
		dst.m[3][2] = z;
	}

	void matrix_ortho_lh(D3DMATRIX& dst, float w, float h, float zn, float zf)
	{
		matrix_identity(dst);
		dst.m[0][0] = 2.0f / w;
		dst.m[1][1] = 2.0f / h;
		dst.m[2][2] = 1.0f / (zf - zn);
		dst.m[3][2] = zn / (zn - zf);
	}

	void matrix_rotate_z(D3DMATRIX& dst, float angle)
	{
		matrix_identity(dst);
		dst.m[0][0] = cosf(angle);
		dst.m[1][1] = cosf(angle);
		dst.m[0][1] = sinf(angle);
		dst.m[1][0] = -sinf(angle);
	}
		

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
			, texture_(to_com_ptr(texture))
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
		uint32_t width_;
		uint32_t height_;

	public:

		FrameBuffer(
			shared_ptr<IDirect3DDevice9Ex> const device,
			vector<shared_ptr<Texture2D>> const& buffers)
			: device_(device)
			, width_(0)
			, height_(0)
		{
			buffers_.assign(buffers.begin(), buffers.end());

			if (!buffers_.empty()) {
				width_ = buffers.front()->width();
				height_ = buffers.front()->height();			
			}
		}

		uint32_t width() const { return width_; }
		uint32_t height() const { return height_; }

		shared_ptr<Texture2D> bind(void* target)
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

			return texture;
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

	class Quad
	{
	private:
		shared_ptr<IDirect3DDevice9Ex> const device_;
		shared_ptr<IDirect3DVertexBuffer9> const vb_;

	public:
		Quad(shared_ptr<IDirect3DDevice9Ex> const& device,
			IDirect3DVertexBuffer9* vb)
			: device_(device)
			, vb_(to_com_ptr(vb))
		{
		}
		
		void draw(shared_ptr<Texture2D> const& texture)
		{
			if (texture)
			{
				device_->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
				device_->SetTexture(0, *texture);
			}
			else
			{
				device_->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
				device_->SetTexture(0, nullptr);
			}

			device_->SetStreamSource(0, vb_.get(), 0, sizeof(VERTEX));
			device_->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		}
	};

	

	class Renderer : public IScene
	{
	private:
		shared_ptr<IAssets> const assets_;
		shared_ptr<IDirect3DDevice9Ex> const device_;
		shared_ptr<FrameBuffer> const frame_buffer_;
		shared_ptr<ISurfaceQueue> const queue_;
		shared_ptr<IDirect3DQuery9> flush_query_;
		
		shared_ptr<Quad> bg_quad_;
		shared_ptr<Texture2D> bg_;

		shared_ptr<Quad> preview_quad_;
		shared_ptr<Quad> spinner_quad_;
		
		shared_ptr<Quad> pattern_quad_;
		shared_ptr<Texture2D> pattern_;
		
		float spin_angle_;

	public:
		Renderer(
			shared_ptr<IAssets> const& assets,
			shared_ptr<IDirect3DDevice9Ex> const& device,
			shared_ptr<FrameBuffer> const& frame_buffer,
			shared_ptr<ISurfaceQueue> const& queue)
			: assets_(assets)
			, device_(device)
			, frame_buffer_(frame_buffer)
			, queue_(queue)
		{
			spin_angle_ = 0.0f;
			device_->SetRenderState(D3DRS_LIGHTING, 0);
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

		uint32_t width() const { 
			return frame_buffer_ ? frame_buffer_->width() : 0;
		}
		
		uint32_t height() const { 
			return frame_buffer_ ? frame_buffer_->height() : 0;
		}

		void tick(double) override
		{
			spin_angle_ = spin_angle_ + static_cast<float>(1 * (PI / 180.0));
		}

		void clear(float red, float green, float blue, float alpha)
		{
			auto const color = D3DCOLOR_COLORVALUE(red, green, blue, alpha);
			device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
		}

		void render() override
		{
			auto const target = queue_->checkout(100);
			if (!target) {
			}
			
			clear(0.0f, 0.0f, 0.0f, 0.0f);
			
			device_->BeginScene();

			device_->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			
			auto const w = width();
			auto const h = height();
			
			D3DMATRIX mview;
			matrix_identity(mview);
			mview._41 = (-(w / 2.0f));
			mview._42 = (-(h / 2.0f));

			D3DMATRIX mproj;
			matrix_ortho_lh(mproj, w * 1.0f, h * -1.0f, 0.0f, 1.0f);

			device_->SetTransform(D3DTS_PROJECTION, &mproj);
			device_->SetTransform(D3DTS_VIEW, &mview);

			shared_ptr<Texture2D> buffer;
				
			{
				buffer = frame_buffer_->bind(target->share_handle());
					
				clear(0.0f, 0.0f, 0.0f, 0.0f);
	
				render_scene();
					
				frame_buffer_->unbind();
			}
			
			// we could add a property to disable preview
			preview(buffer);

			device_->EndScene();

			// ensure the D3D9 device is done writing to our texture buffer
			flush();

			// place on queue so a producer will be notified
			queue_->produce(target);			
		}

		void present(int32_t) override
		{
			device_->Present(nullptr, nullptr, nullptr, nullptr);
		}

		shared_ptr<ISurfaceQueue> queue() const {
			return queue_;
		}

		//
		// render the surface to our window swapchain so we can 
		// preview it on-screen
		//
		void preview(shared_ptr<Texture2D> const& texture)
		{
			if (!preview_quad_) {
				preview_quad_ = create_quad(0.0f, 0.0f, float(width()), float(height()));
			}

			// load pattern to show transparency
			if (!pattern_) 
			{
				pattern_ = load_texture("transparent.png");
				if (!pattern_quad_)
				{
					if (pattern_) 
					{
						float u = width() / float(pattern_->width());
						float v = height() / float(pattern_->height());
						pattern_quad_ = create_quad(0.0f, 0.0f, float(width()), float(height()), u, v);
					}			
				}
			}

			D3DMATRIX mworld;
			matrix_identity(mworld);
			device_->SetTransform(D3DTS_WORLD, &mworld);

			// draw transparency pattern if we have one
			if (pattern_quad_ && pattern_)
			{
				device_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				device_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				enable_blending(false);
				pattern_quad_->draw(pattern_);
			}

			// draw the current frame to our preview (window swap chain)
			if (preview_quad_)
			{
				device_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				device_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				enable_blending(false);
				preview_quad_->draw(texture);
			}
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

		shared_ptr<Quad> create_quad(
			float x, float y, float w, float h, float u=1.0f, float v=1.0f)
		{
			IDirect3DVertexBuffer9* vb = nullptr;
			auto const hr = device_->CreateVertexBuffer(
				4 * sizeof(VERTEX),
				0,
				D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
				D3DPOOL_DEFAULT,
				&vb,
				nullptr);
			if (FAILED(hr)) {
				return nullptr;
			}

			auto color = D3DCOLOR_XRGB(0xff, 0xff, 0xff);

			VERTEX vertices[] =
			{
				{ x, y, 0.5f,         color, 0.0f, 0.0f },
				{ x + w, y, 0.5f,     color, u, 0.0f },
				{ x, y + h, 0.5f,     color, 0.0f, v },
			   { x + w, y + h, 0.5f, color, u, v }
			};

			void* p = nullptr;

			vb->Lock(0, 0, (void**)&p, 0);
			memcpy(p, vertices, sizeof(vertices));
			vb->Unlock();

			return make_shared<Quad>(device_, vb);
		}

		void enable_blending(bool enable)
		{
			device_->SetRenderState(D3DRS_ALPHABLENDENABLE, enable ? 1 : 0);
			device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}

		void render_scene()
		{
			// load background image
			if (!bg_)
			{
				bg_ = load_texture("d3d9_bg.png");
				if (!bg_quad_ && bg_) {
					bg_quad_ = create_quad(0.0f, 0.0f, float(width()), float(height()));
				}
			}

			if (!spinner_quad_) 
			{
				float bar_w = height() * 0.75f;
				float bar_h = 20.0f;

				spinner_quad_ = create_quad(
					0.0f - (bar_w / 2), 0.0f - (bar_h / 2), bar_w, bar_h);
			}

			D3DMATRIX mworld;
			matrix_identity(mworld);

			device_->SetTransform(D3DTS_WORLD, &mworld);

			// draw background image if we have one
			if (bg_quad_ && bg_)
			{
				device_->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				device_->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				enable_blending(true);
				bg_quad_->draw(bg_);
			}

			D3DMATRIX mtrans;
			matrix_translation(mtrans,
				(width() / 2.0f), 
				(height() / 2.0f), 0.0f);

			D3DMATRIX mrotate;
			matrix_identity(mrotate);

			matrix_rotate_z(mrotate, spin_angle_);			

			matrix_multiply(mworld, mrotate, mtrans);

			device_->SetTransform(D3DTS_WORLD, &mworld);

			if (spinner_quad_) {
				spinner_quad_->draw(nullptr);
			}
		}

		shared_ptr<Texture2D> load_texture(string const& key)
		{
			if (!assets_) {
				return nullptr;
			}

			auto const path = assets_->locate(key);
			auto const img = assets_->load_image(path);
			if (!img) {
				return nullptr;
			}

			auto const w = img->width();
			auto const h = img->height();

			IDirect3DTexture9* texture = nullptr;			
			auto hr = device_->CreateTexture(
				w, h, 1,
				D3DUSAGE_DYNAMIC,
				D3DFMT_A8R8G8B8,
				D3DPOOL_DEFAULT,
				&texture,
				nullptr);
			if (FAILED(hr)) {
				return nullptr;
			}

			D3DLOCKED_RECT lock;
			hr = texture->LockRect(0, &lock, nullptr, D3DLOCK_NOSYSLOCK);
			if (SUCCEEDED(hr)) 
			{
				uint32_t src_stride;
				uint8_t* src = reinterpret_cast<uint8_t*>(img->lock(src_stride));
				if (src)
				{
					if (src_stride == lock.Pitch)
					{
						memcpy(lock.pBits, src, lock.Pitch * h);					
					}
					else 
					{
						size_t cb = src_stride < uint32_t(lock.Pitch) ? src_stride : lock.Pitch;
						uint8_t* dst = reinterpret_cast<uint8_t*>(lock.pBits);
						for (size_t y = 0; y < h; ++y)
						{
							memcpy(dst, src, cb);
							src += src_stride;
							dst += lock.Pitch;
						}
					}
					img->unlock();				
				}
				texture->UnlockRect(0);
			}

			return make_shared<Texture2D>(device_, texture, nullptr);
		}

	};
}

//
// create a D3D9Ex device instance
//
shared_ptr<IDirect3DDevice9Ex> create_device(HWND window, uint32_t width, uint32_t height)
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
	pp.BackBufferHeight = height;
	pp.BackBufferWidth = width;

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
			width, height, 1, 
			D3DUSAGE_RENDERTARGET, 
			D3DFMT_A8R8G8B8, 
			D3DPOOL_DEFAULT, 
			&texture, 
			&share);
		if (SUCCEEDED(hr)) {
			textures.push_back(make_shared<Texture2D>(device, texture, share));
		}
	}

	if (!textures.empty()) {
		return make_shared<FrameBuffer>(device, textures);
	}
	return nullptr;
}

shared_ptr<IScene> create_producer(
	void* native_window, uint32_t width, uint32_t height, shared_ptr<IAssets> const& assets)
{
	auto const dev = create_device((HWND)native_window, width, height);
	if (!dev) {
		return nullptr;
	}

	// create shared buffers for delivery to a consumer
	auto const swapchain = create_frame_buffer(dev, 3, width, height);
	if (!swapchain) {
		return nullptr;
	}
	
	// notify the surface queue about the shared textures 
	// we will be rendering to
	auto const queue = create_surface_queue();
	for (size_t n = 0; n < swapchain->buffer_count(); ++n) {
		queue->checkin(swapchain->buffer(n));			
	}
	
	auto const producer = make_shared<Renderer>(assets, dev, swapchain, queue);
	
	string title("Direct3D9 Producer");
	title.append(" - [gpu: ");
	title.append(producer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return producer;
}


