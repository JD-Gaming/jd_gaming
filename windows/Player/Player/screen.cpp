#include "screen.h"

#include <Windows.h>
#include <gl/gl.h>

Screen::Screen(unsigned int width, unsigned int height, const char *title) :
	w(width),
	h(height)
{
	pixels = new unsigned char[width * height];
	memset(&wc, 0, sizeof(wc));
	wc.lpfnWndProc = DefWindowProc;
	wc.style = CS_CLASSDC;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = TEXT("wincls");
	RegisterClassEx(&wc);
	hwnd = CreateWindow(TEXT("wincls"), TEXT(title?title:"Generic window"), WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, wc.hInstance, NULL);
	pfd = { 0 }; pfd.dwFlags = PFD_DOUBLEBUFFER;
	SetPixelFormat(GetDC(hwnd), ChoosePixelFormat(GetDC(hwnd), &pfd), &pfd);
	ctx = wglCreateContext(GetDC(hwnd)); wglMakeCurrent(GetDC(hwnd), ctx);
	BOOL(__stdcall* SwapInterval) (int) = (BOOL(__stdcall*)(int)) wglGetProcAddress("wglSwapIntervalEXT");
	if (SwapInterval) {
		SwapInterval(1);
	}

	glGenTextures(1, &backbuffer);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, backbuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

Screen::~Screen()
{
	wglMakeCurrent(GetDC(hwnd), 0);
	wglDeleteContext(ctx);
	DestroyWindow(hwnd);

	delete[]pixels;
}

void Screen::swap(void)
{
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0); glVertex2f(-1, -1); glTexCoord2f(1, 0); glVertex2f(1, -1);
	glTexCoord2f(1, 1); glVertex2f(1, 1); glTexCoord2f(0, 1); glVertex2f(-1, 1);
	glEnd();
	SwapBuffers(GetDC(hwnd));

	MSG msg;
	while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

}
