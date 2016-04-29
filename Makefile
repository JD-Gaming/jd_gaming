GCC=gcc

LIBDIR=libs

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR)

LDFLAGS = \
	-L$(LIBDIR) -lcanvas -lbmp -ljpeg -lm -lz -lpthread

all: game

game: player.o mygame.o $(LIBDIR)/libcanvas.a
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

player.o: player.c mygame.h game.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

mygame.o: mygame.c mygame.h game.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

clean:
	echo "[RM] $^"
	-rm *.o game

.SILENT:
