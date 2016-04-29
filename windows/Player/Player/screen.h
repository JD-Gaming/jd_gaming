#ifndef SCREEN_H
#define SCREEN_H

#include <Windows.h>
#include <gl/gl.h>

class Screen {
public:
	Screen(const unsigned int width, const unsigned int height, const char *title=NULL);
	~Screen();

	// Swap buffer and update the screen
	void swap(void);

	unsigned char *pixels;

private:
	WNDCLASSEX wc;
	HWND hwnd;
	PIXELFORMATDESCRIPTOR pfd;
	HGLRC ctx;
	GLuint backbuffer;

	unsigned int w;
	unsigned int h;
};

#endif
