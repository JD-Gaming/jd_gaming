GCC=gcc
LIBDIR=libs

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR)

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

player.o: player.c arkanoid.h game.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

arkanoid.o: arkanoid.c arkanoid.h game.h geometry.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

geometry.o: geometry.c geometry.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

clean:
	echo "[RM] $^"
	-rm *.o game${EXT}

.SILENT:
