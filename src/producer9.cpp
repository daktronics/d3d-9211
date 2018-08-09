#include "scene.h"
#include "util.h"

#include "d3d9.h"

using namespace std;

namespace {

	class Renderer : public Producer
	{
	private:
		shared_ptr<d3d9::Device> const device_;

	public:
		Renderer(shared_ptr<d3d9::Device> const& device)
			: device_(device)
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
		}

		void present() override
		{
		}

	};

}


shared_ptr<Producer> create_producer(void* native_window)
{
	auto const dev = d3d9::create_device((HWND)native_window);
	if (!dev) {
		return nullptr;
	}
	
	auto const producer = make_shared<Renderer>(dev);

	string title("Direct3D9 Producer");
	title.append(" - [gpu: ");
	title.append(producer->gpu());
	title.append("]");
	SetWindowText((HWND)native_window, to_utf16(title).c_str());

	return producer;
}


