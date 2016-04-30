#ifndef SCREEN_H
#define SCREEN_H

#include <Windows.h>
#include <gl/gl.h>
#include <stdint.h>

class Screen {
public:
	Screen(const uint32_t width, const uint32_t height, const char *title=NULL);
	~Screen();

	const uint32_t getWidth(void);
	const uint32_t getHeight(void);

	float getFps(void);
	bool getVSync(void);
	void toggleVSync(bool on);

	// Swap buffer and update the screen
	void swap(void);

public:
	// This is an ugly hack to be able to write quickly to the frame buffer
	uint32_t *pixels;

private:
	WNDCLASSEX wc;
	HWND hwnd;
	PIXELFORMATDESCRIPTOR pfd;
	HGLRC ctx;
	GLuint backbuffer;

	uint32_t w;
	uint32_t h;

	float fps;
	uint32_t frameCount;
	LARGE_INTEGER perfFrequency;
	LARGE_INTEGER previousTime;

	bool vsyncOn;

private:
	void calculateFps(void);
};

#endif
