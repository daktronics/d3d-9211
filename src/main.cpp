#include "platform.h"
#include "util.h"
#include "scene.h"

#include "resource.h"

#include <fstream>
#include <sstream>

using namespace std;

//
// if we're running on a system with hybrid graphics ... 
// try to force the selection of the high-performance gpu
//
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
			
HWND create_window(HINSTANCE instance);
void zoom_window(HWND window, shared_ptr<IScene> const& scene, float zoom);

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

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int)
{
	// this demo uses WIC to load images .. so we need COM
	ComInitializer com_init;

	uint32_t width = 1920;
	uint32_t height = 1080;

	// create window(s) with our specific size
	auto const win_main = create_window(instance);
	if (!IsWindow(win_main)) {
		assert(0);
		return 0;
	}

	auto const win_preview = create_window(instance);
	if (!IsWindow(win_preview)) {
		assert(0);
		return 0;
	}

	auto const producer = create_producer(win_preview, width, height);
	auto const consumer = create_consumer(win_main, width, height, producer);

	// make the windows visible now that we have D3D components ready
	zoom_window(win_main, consumer, 0.5f);
	zoom_window(win_preview, producer, 0.5f);

	// load keyboard accelerators
	auto const accel_table = 
		LoadAccelerators(instance, MAKEINTRESOURCE(IDR_APPLICATION));

	auto const start_time = time_now();

	// main message pump for our application
	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(win_main, accel_table, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			auto const t = (time_now() - start_time) / 1000000.0;

			producer->tick(t);
			producer->render();

			consumer->tick(t);
			consumer->render();

			// our preview window shows the producer ... without vsync
			producer->present(0);

			// our main window is vsync'd for the consumer
			consumer->present(sync_interval_);

			// is there a request to resize ... if so, resize
			// both the swapchain and the composition
			/*if (resize_) 
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
			}*/
		}
	}
	
	return 0;
}

void zoom_window(HWND window, shared_ptr<IScene> const& scene, float zoom)
{
	// AdjustWindowRect can do something similar
	RECT rc_outer, rc_inner;
	GetWindowRect(window, &rc_outer);
	GetClientRect(window, &rc_inner);

	auto const w = static_cast<int32_t>(scene->width() * zoom);
	auto const h = static_cast<int32_t>(scene->height() * zoom);
	
	SetWindowPos(window, nullptr, 0, 0,
		w + ((rc_outer.right - rc_outer.left) - (rc_inner.right - rc_inner.left)),
		h + ((rc_outer.bottom - rc_outer.top) - (rc_inner.bottom - rc_inner.top)),
		SWP_NOMOVE | SWP_NOZORDER);

	ShowWindow(window, SW_NORMAL);
}

HWND create_window(HINSTANCE instance)
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

	auto const window = CreateWindow(class_name,
						L"",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 
						nullptr, 
						nullptr, 
						instance, 
						nullptr);

	return window;
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