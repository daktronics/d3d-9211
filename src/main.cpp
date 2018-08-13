#include "platform.h"
#include "util.h"
#include "scene.h"
#include "assets.h"

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
void zoom_to_screen(HWND window);
void zoom_window(HWND window, float zoom);

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

	int args;
	LPWSTR* arg_list = CommandLineToArgvW(GetCommandLineW(), &args);
	if (arg_list)
	{
		for (int n = 1; n < args; ++n)
		{
			auto option = to_utf8(arg_list[n]);
			if (option.substr(0, 2) == "--")
			{
				option = option.substr(2);

				std::string key, value;
				auto const eq = option.find('=');
				if (eq != std::string::npos)
				{
					key = option.substr(0, eq);
					value = option.substr(eq + 1);
				}
				else {
					key = option;
				}
				
				if (key == "size")
				{
					// split on 'x' (eg. 1920x1080)
					auto const c = value.find('x');
					if (c != std::string::npos) {
						width = to_int(value.substr(0, c), 0);
						height = to_int(value.substr(c + 1), 0);
					}
				}
			}
		}
	}

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

	auto assets = create_assets();

	assets->generate(width, height);
	
	auto producer = create_producer(win_preview, width, height, assets);
	auto consumer = create_consumer(win_main, width, height, producer);

	SetWindowLongPtr(win_main, GWLP_USERDATA, (LONG_PTR)consumer.get());
	SetWindowLongPtr(win_preview, GWLP_USERDATA, (LONG_PTR)producer.get());

	zoom_to_screen(win_main);
	zoom_to_screen(win_preview);
	
	// make the windows visible now that we have D3D components ready
	ShowWindow(win_main, SW_NORMAL);
	ShowWindow(win_preview, SW_NORMAL);

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
		}
	}

	// drop before COM is uninitialized
	producer.reset();
	consumer.reset();
	assets.reset();	
	
	return 0;
}

void zoom_to_screen(HWND window)
{
	float zoom = 1.0f;
	auto const mon = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
	if (mon)
	{
		RECT rc;
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(mon, &mi);
		while (zoom > 0.25f)
		{
			zoom_window(window, zoom);
			GetWindowRect(window, &rc);
			if ((rc.right - rc.left) < (mi.rcWork.right - mi.rcWork.left)) {
				if ((rc.bottom - rc.top) < (mi.rcWork.bottom - mi.rcWork.top)) {
					break;
				}
			}
			zoom = zoom * 0.5f;
		}
	}
}

void zoom_window(HWND window, float zoom)
{
	const IScene* scene = (const IScene*)GetWindowLongPtr(window, GWLP_USERDATA);
	if (!scene) {
		return;
	}

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

void on_command(HWND window, uint32_t id)
{
	switch (id) 
	{
		case ID_WINDOW_VSYNC:
			sync_interval_ = sync_interval_ ? 0 : 1;
			resize_ = true;
			break;

		case ID_VIEW_ZOOM25: zoom_window(window, 0.25f); break;
		case ID_VIEW_ZOOM50: zoom_window(window, 0.50f); break;
		case ID_VIEW_ZOOM100: zoom_window(window, 1.0f); break;
		case ID_VIEW_ZOOM200: zoom_window(window, 2.0f); break;

		default: break;
	}
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
			on_command(hwnd, LOWORD(wparam));
			break;

		case WM_SIZE:
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
			
		case WM_CONTEXTMENU:
			{
				auto const menu = LoadMenu(
					GetModuleHandle(0), MAKEINTRESOURCE(IDR_APPLICATION));
				auto const submenu = GetSubMenu(menu, 0);
				auto const x = ((int)(short)LOWORD(lparam));
				auto const y = ((int)(short)HIWORD(lparam));
				TrackPopupMenu(submenu, TPM_LEFTALIGN, x, y, 0, hwnd, 0);
			}
			break;

		default: 
			return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}