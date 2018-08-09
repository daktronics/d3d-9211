#include "scene.h"
#include "util.h"

#include "d3d11.h"

using namespace std;

namespace {

	class Renderer : public Consumer
	{
	private:
		shared_ptr<d3d11::Device> const device_;
		shared_ptr<d3d11::SwapChain> const swapchain_;

	public:
		Renderer(shared_ptr<d3d11::Device> const& device,
			shared_ptr<d3d11::SwapChain> const& swapchain)
			: device_(device)
			, swapchain_(swapchain)
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
			auto const ctx = device_->immedidate_context();

			d3d11::ScopedBinder<d3d11::SwapChain> bind(ctx, swapchain_);

			swapchain_->clear(1.0f, 0.0f, 0.0f, 1.0f);



		}

		void present(int32_t sync_interval) override
		{
			swapchain_->present(sync_interval);
		}

	};

}


shared_ptr<Consumer> create_consumer(void* native_window, 
	shared_ptr<Producer> const& /*producer*/)
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
	
	auto const consumer = make_shared<Renderer>(dev, swapchain);

	string title("Direct3D11 Consumer");
	title.append(" - [gpu: ");
	title.append(consumer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return consumer;
}


