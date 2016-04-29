#include <time.h>
#include <stdio.h>

#include <stdint.h>
#include <math.h>
#include <windows.h>
#pragma comment( lib, "opengl32.lib" )
#include <synchapi.h>

#include "screen.h"


int main() {
	srand((unsigned)(time(NULL)));

	Screen screen(640, 480, "AI world");
}