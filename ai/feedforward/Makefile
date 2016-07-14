GCC=gcc
LIBDIR=../../libs
LIBNAME=libfeedforward.a

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall \
	-std=c99 \
	-Ipcg-c-0.94/include \
	-I$(LIBDIR) -Iinclude

LDFLAGS = -L$(LIBDIR) -L.
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
LDFLAGS += -lffann -ljpeg -lm -lz -lpthread -Lpcg-c-0.94/src -lpcg_random

all: feedforward$(EXT)

feedforward$(EXT): libffann.a main.o
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

libffann.a: network.o layer.o neurons.o activation.o
	echo "[AR] $@"
	ar rcs $@ $^

main.o: main.c neurons.h network.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

activation.o: activation.c activation.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

neurons.o: neurons.c neurons.h activation.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

layer.o: layer.c layer.h neurons.h activation.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

network.o: network.c network.h neurons.h activation.h
	echo "[CC] $@"
	${GCC} $(CCFLAGS) -c $<

clean:
	echo "[RM] $^"
	-rm *.o feedforward${EXT}

.SILENT:
