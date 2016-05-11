GCC=gcc
LIBDIR=libs

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR) -Iinclude -I../include

LDFLAGS = -L$(LIBDIR)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
	LDFLAGS += -lcanvas_cyg -lbmp_cyg
else
	LDFLAGS += -lcanvas -lbmp
endif
LDFLAGS += -ljpeg -lm -lz -lpthread

all: game$(EXT)

game$(EXT): player.o arkanoid.o geometry.o
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

player.o: src/player.c include/arkanoid.h include/game.h
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
