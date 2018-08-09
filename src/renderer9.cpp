#include "scene.h"
#include "util.h"

#include "d3d9.h"

using namespace std;

namespace {

	class Surface : public ISurface
	{
	private:
		shared_ptr<d3d9::Texture2D> texture_;

	public:
		Surface(shared_ptr<d3d9::Texture2D> const& texture) 
			: texture_(texture) {
		}

		uint32_t width() const override {
			return texture_->width();
		}

		uint32_t height() const override {
			return texture_->width();
		}

		void* share_handle() const override {
			return texture_->share_handle();
		}
	};


	class Renderer : public IScene
	{
	private:
		shared_ptr<d3d9::Device> const device_;
		shared_ptr<d3d9::SwapChain> const swapchain_;
		shared_ptr<ISurfaceQueue> const queue_;

	public:
		Renderer(shared_ptr<d3d9::Device> const& device,
			shared_ptr<d3d9::SwapChain> const& swapchain,
			shared_ptr<ISurfaceQueue> const& queue)
			: device_(device)
			, swapchain_(swapchain)
			, queue_(queue)
		{
		}

		string gpu() const override {
			return device_->adapter_name();
		}

		void tick(double) override
		{
		}

		void render() override
		{
			auto const target = queue_->checkout(100);
			if (target)
			{
				device_->clear(0.0f, 0.0f, 0.0f, 0.0f);

				device_->begin_scene();

				{
					swapchain_->bind(target->share_handle());
					device_->clear(0.0f, 0.0f, 1.0f, 1.0f);

					swapchain_->unbind();
				}

				// ensure the D3D9 device is done writing to our texture
				device_->flush();

				// place on queue so a producer will be notified
				queue_->produce(target);
			}

			device_->end_scene();			
		}

		void present(int32_t) override
		{
			device_->present();
		}

		shared_ptr<ISurfaceQueue> queue() const {
			return queue_;
		}

	};
}

shared_ptr<IScene> create_producer(void* native_window)
{
	auto const dev = d3d9::create_device((HWND)native_window);
	if (!dev) {
		return nullptr;
	}

	RECT rc;
	GetClientRect((HWND)native_window, &rc);

	auto const w = rc.right - rc.left;
	auto const h= rc.bottom - rc.top;

	auto const swapchain = dev->create_swapchain(3, w, h);
	if (!swapchain) {
		return nullptr;
	}
		
	auto const queue = create_surface_queue();

	// create shared buffers for delivery to a consumer
	for (size_t n = 0; n < swapchain->buffer_count(); ++n) {
		queue->checkin(make_shared<Surface>(swapchain->buffer(n)));			
	}
	
	auto const producer = make_shared<Renderer>(dev, swapchain, queue);
	
	string title("Direct3D9 Producer");
	title.append(" - [gpu: ");
	title.append(producer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return producer;
}


