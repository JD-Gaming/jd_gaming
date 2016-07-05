GCC=gcc
LIBDIR=libs

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR) -Iinclude

LDFLAGS = -L$(LIBDIR) -L. -Larc/ai/feedforward -larkanoid -lffann -lm  -L3pp/pcg-c-0.94/src -L../3pp/pcg-c-0.94/src -lpcg_random
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
	LDFLAGS_DRAW += -lcanvas_cyg -lbmp_cyg
else ifeq ($(findstring Darwin,$(OSNAME)),Darwin)
	LDFLAGS_DRAW += ../../libs/canvas/canvas.o
	LDFLAGS_DRAW += ../../libs/canvas/get_pixels.o
	LDFLAGS_DRAW += ../../libs/canvas/set_pixels.o
	LDFLAGS_DRAW += ../../libs/canvas/jpeg.o
	LDFLAGS_DRAW += ../../libs/canvas/bmp.o
	LDFLAGS_DRAW += ../../libs/canvas/pnm.o
	LDFLAGS_DRAW += ../../libs/bmp/bmp.o
else
	LDFLAGS_DRAW += -lcanvas -lbmp
endif
LDFLAGS_DRAW += -ljpeg -lz -lpthread

FFANNLIB=src/ai/feedforward/libffan.a
ARKANOIDLIB=src/game/arkanoid/libarkanoid.a


all: game$(EXT) render$(EXT)

game$(EXT): player.o population.o $(LIBNAME)
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

render$(EXT): render.o $(LIBNAME)
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) $(LDFLAGS_DRAW) -o $@

$(LIBNAME): arkanoid.o geometry.o
	echo "[AR] $@"
	ar rcs $@ $^

player.o: src/player.c include/arkanoid.h include/game.h ai/feedforward/network.h src/population.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

render.o: src/render.c include/arkanoid.h include/game.h ai/feedforward/network.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

population.o: src/population.c src/population.h ai/feedforward/network.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

arkanoid.o: src/arkanoid.c include/arkanoid.h include/game.h include/geometry.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

geometry.o: src/geometry.c include/geometry.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

clean:
	echo "[RM] $^"
	-rm *.o game${EXT}

.SILENT:
