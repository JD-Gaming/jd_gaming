GCC=gcc
LIBDIR=libs
LIBNAME=libarkanoid.a

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR) -Iinclude -I../include -Iai/feedforward -I../ai/feedforward

LDFLAGS = -L$(LIBDIR) -L. -Lai/feedforward -larkanoid -lffann -lm  -Lai/feedforward/pcg-c-0.94/src -L../ai/feedforward/pcg-c-0.94/src -lpcg_random -ljobhandler -lprogress
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


all: game$(EXT) render$(EXT) addTrainer$(EXT) inspectNet$(EXT)

game$(EXT): player.o population.o $(LIBNAME)
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

addTrainer$(EXT): addTrainer.o population.o
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

inspectNet$(EXT): inspectNet.o
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

addTrainer.o: src/addTrainer.c ai/feedforward/network.h src/population.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

inspectNet.o: src/inspectNet.c ai/feedforward/network.h
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
