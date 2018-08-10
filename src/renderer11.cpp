#include "scene.h"
#include "util.h"

#include "d3d11.h"
#include <map>

using namespace std;

namespace {

	class Renderer : public IScene
	{
	private:
		shared_ptr<d3d11::Device> const device_;
		shared_ptr<d3d11::SwapChain> const swapchain_;
		shared_ptr<ISurfaceQueue> const queue_;
		shared_ptr<d3d11::Geometry> geometry_;
		shared_ptr<d3d11::Effect> effect_;		
		shared_ptr<ISurface> surface_;
		map<void*, shared_ptr<d3d11::Texture2D>> textures_;		

	public:
		Renderer(shared_ptr<d3d11::Device> const& device,
			shared_ptr<d3d11::SwapChain> const& swapchain,
			shared_ptr<ISurfaceQueue> const& queue)
			: device_(device)
			, swapchain_(swapchain)
			, queue_(queue)
		{
		}

		string gpu() const override {
			return device_->adapter_name();
		}

		uint32_t width() const override {
			return swapchain_ ? swapchain_->width() : 0;
		}

		uint32_t height() const override {
			return swapchain_ ? swapchain_->height() : 0;
		}

		void tick(double) override
		{
		}

		void render() override
		{
			auto const ctx = device_->immedidate_context();

			d3d11::ScopedBinder<d3d11::SwapChain> bind(ctx, swapchain_);

			swapchain_->clear(1.0f, 0.0f, 0.0f, 1.0f);

			if (!geometry_) {
				geometry_ = device_->create_quad(0.0f, 0.0f, 1.0f, 1.0f, false);
			}

			auto const surface = queue_->consume(100);
			if (surface)
			{
				surface_ = surface;

				void* handle = surface->share_handle();

				shared_ptr<d3d11::Texture2D> texture;
				auto const i = textures_.find(handle);
				if (i != textures_.end()) 
				{
					texture = i->second;
				}
				else 
				{
					texture = device_->open_shared_texture(handle);
					if (texture) {
						textures_[handle] = texture;
					}
				}

				if (geometry_ && texture)
				{
					// we need a shader
					if (!effect_) {
						effect_ = device_->create_default_effect();
					}

					// bind our states/resource to the pipeline
					d3d11::ScopedBinder<d3d11::Geometry> quad_binder(ctx, geometry_);
					d3d11::ScopedBinder<d3d11::Effect> fx_binder(ctx, effect_);
					d3d11::ScopedBinder<d3d11::Texture2D> tex_binder(ctx, texture);

					// actually draw the quad
					geometry_->draw();
				}			
			}
		}

		void present(int32_t sync_interval) override
		{
			swapchain_->present(sync_interval);

			queue_->checkin(surface_);
		}

		shared_ptr<ISurfaceQueue> queue() const {
			return queue_;
		}

	};

}


shared_ptr<IScene> create_consumer(
	void* native_window, 
	uint32_t width,
	uint32_t height,
	shared_ptr<IScene> const& producer)
{
	auto const dev = d3d11::create_device();
	if (!dev) {
		return nullptr;
	}

	// create a D3D11 swapchain for the window
	auto swapchain = dev->create_swapchain((HWND)native_window);
	if (!swapchain) {
		return nullptr;
	}
	
	auto const consumer = make_shared<Renderer>(
		dev, swapchain, producer->queue());

	string title("Direct3D11 Consumer");
	title.append(" - [gpu: ");
	title.append(consumer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return consumer;
}


