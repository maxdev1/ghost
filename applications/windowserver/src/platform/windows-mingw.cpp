#ifdef _WIN32
#include <windowserver.hpp>
#include <components/cursor.hpp>

#include <windows.h>
#include <cairo/cairo.h>
#include <cairo/cairo-win32.h>
#include <dirent.h>

static const int WIDTH = 800, HEIGHT = 600;
static unsigned int pixels[WIDTH * HEIGHT];
static BITMAPINFO bmi;
static HWND hwnd;
static bool running = true;
static int mouseX = 0, mouseY = 0;
static bool mouseDown = false;

windowserver_t* server = nullptr;

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	switch(m)
	{
		case WM_CREATE:
			ShowCursor(FALSE);
			return 0;
		case WM_DESTROY:
			ShowCursor(TRUE);
			running = false;
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
			if(w == VK_ESCAPE)
			{
				running = false;
				DestroyWindow(h);
			}
			return 0;
		case WM_MOUSEMOVE:
			mouseX = (short) (l & 0xFFFF);
			mouseY = (short) ((l >> 16) & 0xFFFF);
			return 0;
		case WM_LBUTTONDOWN:
			mouseDown = true;
			return 0;
		case WM_LBUTTONUP:
			mouseDown = false;
			return 0;
	}
	return DefWindowProc(h, m, w, l);
}

void startServer()
{
	printf("In startServer\n");
	server->launch();
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int)
{
	WNDCLASS wc = {};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.lpszClassName = "SimpleCairoWin";
	RegisterClass(&wc);

	hwnd = CreateWindowEx(0, wc.lpszClassName, "ghost windowserver", WS_OVERLAPPEDWINDOW,
	                      CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, 0, 0, inst, 0);
	ShowWindow(hwnd, SW_SHOW);

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = WIDTH;
	bmi.bmiHeader.biHeight = -HEIGHT;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC dc = GetDC(hwnd);

	cairo_surface_t* surface = cairo_image_surface_create_for_data(
			(unsigned char*) pixels, CAIRO_FORMAT_RGB24, WIDTH, HEIGHT, WIDTH * 4);
	cairo_t* cr = cairo_create(surface);

	server = new windowserver_t();
	platformCreateThread((void*) &startServer);

	MSG msg;
	while(running)
	{
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// cairo_save(cr);
		// cairo_set_source_rgb(cr, 0.125, 0.125, 0.125);
		// cairo_paint(cr); // background
		//
		// int size = 20;
		// cairo_rectangle(cr, mouseX - size, mouseY - size, size * 2, size * 2);
		// if(mouseDown)
		// 	cairo_set_source_rgb(cr, 0, 1, 0);
		// else
		// 	cairo_set_source_rgb(cr, 1, 0, 0);
		// cairo_fill(cr);
		// cairo_restore(cr);

		StretchDIBits(dc, 0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT,
		              pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
		Sleep(16);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
	ReleaseDC(hwnd, dc);
	return 0;
}

class windows_video_output_t : public g_video_output
{
public:
	explicit windows_video_output_t() = default;

	bool initialize() override
	{
		return true;
	}

	void blit(g_rectangle invalid, g_rectangle sourceSize, g_color_argb* source) override
	{
		memcpy(pixels, source, WIDTH * HEIGHT * sizeof(uint32_t));
		// int x0 = std::max(0, invalid.x);
		// int y0 = std::max(0, invalid.y);
		// int x1 = std::min(WIDTH, invalid.x + invalid.width);
		// int y1 = std::min(HEIGHT, invalid.y + invalid.height);
		//
		// for (int y = y0; y < y1; ++y)
		// {
		// 	uint32_t* dstRow = pixels + y * WIDTH + x0;
		// 	g_color_argb* srcRow = source + (y - invalid.y) * sourceSize.width + (x0 - invalid.x);
		// 	memcpy(dstRow, srcRow, (x1 - x0) * sizeof(uint32_t));
		// }
	}

	g_dimension getResolution() override
	{
		return g_dimension(WIDTH, HEIGHT);
	}
};

void platformMouseReceiver()
{
	int lastX =mouseX;
	int lastY = mouseY;
	bool lastMouseDown = mouseDown;
	while(true)
	{
		if(lastX != mouseX || lastY != mouseY || lastMouseDown != mouseDown)
		{
			cursor_t::nextPosition.x = mouseX;
			cursor_t::nextPosition.y = mouseY;

			if(cursor_t::nextPosition.x < 0)
			{
				cursor_t::nextPosition.x = 0;
			}
			if(cursor_t::nextPosition.x > WIDTH - 2)
			{
				cursor_t::nextPosition.x = WIDTH - 2;
			}
			if(cursor_t::nextPosition.y < 0)
			{
				cursor_t::nextPosition.y = 0;
			}
			if(cursor_t::nextPosition.y > HEIGHT - 2)
			{
				cursor_t::nextPosition.y = HEIGHT - 2;
			}

			cursor_t::nextPressedButtons = G_MOUSE_BUTTON_NONE;
			if(mouseDown)
			{
				cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_1;
			}
			// if(info.button2)
			// {
			// 	cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_2;
			// }
			// if(info.button3)
			// {
			// 	cursor_t::nextPressedButtons |= G_MOUSE_BUTTON_3;
			// }

			// cursor_t::nextScroll += info.scroll;

			windowserver_t::instance()->requestUpdateImmediately();

			lastX = mouseX;
			lastY = mouseY;
			lastMouseDown = mouseDown;
		}
		platformSleep(10);
	}
}

void platformStartInput()
{
	platformCreateThread((void*) &platformMouseReceiver);
}

void platformLog(const char* message, ...)
{
	va_list l;
	va_start(l, message);
	vprintf(message, l);
	va_end(l);
	printf("\n");
}


g_video_output* platformCreateVideoOutput()
{
	return new windows_video_output_t();
}

SYS_MUTEX_T platformInitializeMutex(bool reentrant)
{
	win_mutex_t* mutex = (win_mutex_t*) malloc(sizeof(win_mutex_t));
	mutex->reentrant = reentrant;
	if(reentrant)
		InitializeCriticalSection(&mutex->cs);
	else
		mutex->handle = CreateSemaphore(NULL, 1, 1, NULL);
	return mutex;
}

void platformAcquireMutex(SYS_MUTEX_T m)
{
	if(m->reentrant)
	{
		EnterCriticalSection(&m->cs);
	}
	else
	{
		WaitForSingleObject(m->handle, INFINITE);
	}
}

void platformReleaseMutex(SYS_MUTEX_T m)
{
	if(m->reentrant)
	{
		LeaveCriticalSection(&m->cs);
	}
	else
	{
		ReleaseSemaphore(m->handle, 1, NULL);
	}
}

void platformAcquireMutexTimeout(SYS_MUTEX_T mutex, uint32_t timeout)
{
	// TODO
}

SYS_TX_T platformCreateMessageTransaction()
{
	printf("NOT IMPLEMENTED: platformCreateMessageTransaction\n");
}

bool platformSendMessage(SYS_TID_T tid, void* buf, size_t len, SYS_TX_T tx)
{
	printf("NOT IMPLEMENTED: platformSendMessage\n");
}

int platformReceiveMessage(void* buf, size_t max, SYS_TX_T tx)
{
	printf("NOT IMPLEMENTED: platformReceiveMessage\n");

	for(;;) platformSleep(1000);
}

void platformYieldTo(SYS_TID_T tid)
{
	printf("NOT IMPLEMENTED: platformYieldTo\n");
}

void platformUnmapSharedMemory(void* mem)
{
	printf("NOT IMPLEMENTED: platformUnmapSharedMemory\n");
}

DWORD WINAPI _platformThreadEntry(LPVOID arg)
{
	void (*entry)() = (void (*)()) arg;
	entry();
	return 0;
}

SYS_TID_T platformCreateThread(void* entry)
{
	return CreateThread(NULL, 0, _platformThreadEntry, entry, 0, NULL);
}

SYS_TID_T platformCreateThreadWithData(void* entry, void* data)
{
	printf("NOT IMPLEMENTED: platformCreateThreadWithData\n");
}

SYS_TID_T platformGetPidForTid(SYS_TID_T tid)
{
	printf("NOT IMPLEMENTED: platformGetPidForTid\n");
}

void* platformAllocateMemory(size_t size)
{
	printf("NOT IMPLEMENTED: platformAllocateMemory\n");
	return nullptr;
}

void* platformShareMemory(void* memory, size_t size, SYS_TID_T target)
{
	printf("NOT IMPLEMENTED: platformShareMemory\n");
	return nullptr;
}

char platformCharForKey(g_key_info key)
{
	return 'A';
}

uint64_t platformMillis()
{
	return GetTickCount();
}

void platformJoin(SYS_TID_T tid)
{
	printf("NOT IMPLEMENTED: platformJoin\n");
}

bool platformRegisterTaskIdentifier(const char* task)
{
	printf("NOT IMPLEMENTED: platformRegisterTaskIdentifier\n");
	return true;
}

SYS_TID_T platformSpawn(const char* path, const char* args, const char* cwd)
{
	printf("NOT IMPLEMENTED: platformSpawn\n");
	return nullptr;
}

bool platformInitializeKeyboardLayout(std::string layout)
{
	return true;
}

void platformSleep(uint64_t time)
{
	Sleep(time);
}

SYS_TID_T platformGetTid()
{
	printf("NOT IMPLEMENTED: platformGetTid\n");
	return nullptr;
}

void platformExit(int v)
{
	printf("NOT IMPLEMENTED: platformExit\n");
}


void platformLoadCursors()
{
	const char *path = "../../sysroot/system/graphics/cursor";
	DIR *dir = opendir(path);
	if (!dir) {
		return;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string path = std::string("../../sysroot/system/graphics/cursor") + "/" + entry->d_name;
		cursor_t::load(path);
	}
	cursor_t::set("default");
	closedir(dir);
}



#endif
