GCC=gcc
LIBDIR=../../libs
LIBNAME=libfeedforward.a

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall -O3 \
	-I$(LIBDIR) -Iinclude

LDFLAGS = -L$(LIBDIR)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
	LDFLAGS += -lcanvas_cyg -lbmp_cyg
else ifeq ($(findstring Darwin,$(OSNAME)),Darwin)
	LDFLAGS += ../../../../libs/canvas/canvas.o
	LDFLAGS += ../../../../libs/canvas/get_pixels.o
	LDFLAGS += ../../../../libs/canvas/set_pixels.o
	LDFLAGS += ../../../../libs/canvas/jpeg.o
	LDFLAGS += ../../../../libs/canvas/bmp.o
	LDFLAGS += ../../../../libs/canvas/pnm.o
	LDFLAGS += ../../../../libs/bmp/bmp.o
else
	LDFLAGS += -lcanvas -lbmp
endif
LDFLAGS += -ljpeg -lm -lz -lpthread

all: feedforward$(EXT)

feedforward$(EXT): neurons.o network.o main.o
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

main.o: main.c neurons.h network.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

neurons.o: neurons.c neurons.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

network.o: network.c network.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

clean:
	echo "[RM] $^"
	-rm *.o feedforward${EXT}

.SILENT:
