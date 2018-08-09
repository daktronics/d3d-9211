#pragma once

#include <d3d9.h>
#include <memory>
#include <string>

namespace d3d9 {

	class SwapChain;
	class Geometry;
	class Effect;
	class Texture2D;
	class Context;

	//
	// encapsulate a D3D11 Device object
	//
	class Device
	{
	private:
		HMODULE _lib_compiler;
		std::shared_ptr<IDirect3DDevice9Ex> const device_;

	public:
		Device(IDirect3DDevice9Ex*);

		std::string adapter_name() const;

		operator IDirect3DDevice9Ex*() {
			return device_.get();
		}

		void clear(float red, float green, float blue, float alpha);
		void present();

		//std::shared_ptr<SwapChain> create_swapchain(HWND, int width=0, int height=0);
		
		//std::shared_ptr<Geometry> create_quad(
		//			float x, float y, float width, float height, bool flip=false);

		/*std::shared_ptr<Texture2D> create_texture(
					int width, 
					int height, 
					DXGI_FORMAT format, 
					const void* data,
					size_t row_stride);*/

		//std::shared_ptr<Texture2D> open_shared_texture(void*);

		/*std::shared_ptr<Effect> create_default_effect();

		std::shared_ptr<Effect> create_effect(
						std::string const& vertex_code,
						std::string const& vertex_entry,
						std::string const& vertex_model,
						std::string const& pixel_code,
						std::string const& pixel_entry,
						std::string const& pixel_model);*/

	private:

		//std::shared_ptr<ID3DBlob> compile_shader(
		//			std::string const& source_code, 
		//			std::string const& entry_point, 
		//			std::string const& model);

		
	};

	
	std::shared_ptr<Device> create_device(HWND window);
}