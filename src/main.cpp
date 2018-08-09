#include "platform.h"
#include "util.h"

//#include "d3d11.h"
//#include "composition.h"

#include "resource.h"

#include <fstream>
#include <sstream>

//
// if we're running on a system with hybrid graphics ... 
// try to force the selection of the high-performance gpu
//
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
				
				
HWND create_window(HINSTANCE, std::string const& title, int width, int height);
LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);

int sync_interval_ = 1;
bool resize_ = false;

//
// simple RIAA for CoInitialize/CoUninitialize
//
class ComInitializer
{
public:
	ComInitializer() {
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	}
	~ComInitializer() { CoUninitialize(); }
};

//std::shared_ptr<Composition> composition_;

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
	// this demo uses WIC to load images .. so we need COM
	ComInitializer com_init;

	// create a D3D11 rendering device
	//auto device = d3d11::create_device();
	//if (!device) {
	//	assert(0);
	//	cef_uninitialize();
	//	return 0;
	//}

	uint32_t width = 640;
	uint32_t height = 360;

	std::string title("D3D 9 to 11 Demo");
	//title.append(cef_version());

	//title.append(" - [gpu: ");
	//title.append(device->adapter_name());
	//title.append("]");

	// create a window with our specific size
	auto const window = create_window(instance, title, width, height);
	if (!IsWindow(window)) {
		assert(0);
		return 0;
	}

	// create a D3D11 swapchain for the window
	//auto swapchain = device->create_swapchain(window);
	//if (!swapchain) {
	//	assert(0);
	//	return 0;
	//}

	// make the window visible now that we have D3D11 components ready
	ShowWindow(window, SW_NORMAL);

	// load keyboard accelerators
	HACCEL accel_table = 
		LoadAccelerators(instance, MAKEINTRESOURCE(IDR_APPLICATION));

	//auto ctx = device->immedidate_context();

	auto const start_time = time_now();

	// main message pump for our application
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(window, accel_table, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			// update composition + layers based on time
			/*auto const t = (time_now() - start_time) / 1000000.0;
			composition_->tick(t);

			swapchain->bind(ctx);

			// is there a request to resize ... if so, resize
			// both the swapchain and the composition
			if (resize_) 
			{
				RECT rc;
				GetClientRect(window, &rc);
				width = rc.right - rc.left;
				height = rc.bottom - rc.top;
				if (width && height)
				{
					resize_ = false;
					composition_->resize(sync_interval_ != 0, width, height);
					swapchain->resize(width, height);
				}
			}

			// clear the render-target
			swapchain->clear(0.0f, 0.0f, 1.0f, 1.0f);

			// render our scene
			composition_->render(ctx);

			// present to window
			swapchain->present(sync_interval_);*/
		}
	}
	
	return 0;
}

HWND create_window(HINSTANCE instance, std::string const& title, int width, int height)
{
	LPCWSTR class_name = L"_main_window_";

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	if (!GetClassInfoEx(instance, class_name, &wcex))
	{
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = wnd_proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = instance;
		wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOWTEXT + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = class_name;
		wcex.hIconSm = nullptr;
		if (!RegisterClassExW(&wcex)) {
			return nullptr;
		}
	}

	auto const hwnd = CreateWindow(class_name,
						to_utf16(title).c_str(),
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
						nullptr, 
						nullptr, 
						instance, 
						nullptr);

	// AdjustWindowRect can do something similar
	RECT rc_outer, rc_inner;
	GetWindowRect(hwnd, &rc_outer);
	GetClientRect(hwnd, &rc_inner);

	SetWindowPos(hwnd, nullptr, 0, 0, 
			width + ((rc_outer.right - rc_outer.left) - (rc_inner.right - rc_inner.left)),
			height + ((rc_outer.bottom - rc_outer.top) - (rc_inner.bottom - rc_inner.top)),
			SWP_NOMOVE | SWP_NOZORDER);

	return hwnd;
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint(hwnd, &ps);
				EndPaint(hwnd, &ps);
			}
			break;

		case WM_COMMAND:
			if (LOWORD(wparam) == ID_WINDOW_VSYNC) {
				sync_interval_ = sync_interval_ ? 0 : 1;
				resize_ = true;
			}
			break;

		case WM_SIZE: 
			// signal that we want a resize of output
			resize_ = true;
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default: 
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}