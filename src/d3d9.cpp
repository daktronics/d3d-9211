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

	void Device::clear(float red, float green, float blue, float alpha)
	{
		auto const color = D3DCOLOR_COLORVALUE(red, green, blue, alpha);

		device_->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.0f, 0);
	}

	void Device::present()
	{
		device_->Present(nullptr, nullptr, nullptr, nullptr);
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