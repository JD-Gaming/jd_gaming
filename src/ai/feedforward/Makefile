3PPDIR=../../../3pp
INCLUDEDIR=../../../include
LIBNAME=libffann.a
LIBOBJS=neurons.o network.o
OBJS=$(LIBOBJS) main.o

OSNAME:=$(shell uname -a)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall \
	-std=c99 \
	-I$(3PPDIR)/pcg-c-0.94/include \
	-I$(3PPDIR)/libs \
	-I$(INCLUDEDIR)

LDFLAGS = -L$(3PPDIR)/libs -L.
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  ifeq ($(findstring x86_64,$(OSNAME)),x86_64)
	LDFLAGS += -lcanvas_cyg64 -lbmp_cyg64
  else
	LDFLAGS += -lcanvas_cyg -lbmp_cyg
  endif
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
LDFLAGS += -lffann -ljpeg -lm -lz -lpthread -L../../../3pp/pcg-c-0.94/src -lpcg_random

all: $(LIBNAME) feedforward$(EXT)

$(3PPDIR)/pcg-c-0.94/src/libpcg_random.a:
	make -C $(3PPDIR)/pcg-c-0.94/

feedforward$(EXT): $(OBJS) $(3PPDIR)/pcg-c-0.94/src/libpcg_random.a
	echo "[LD] $@"
	${GCC} $(CCFLAGS) $(OBJS) $(LDFLAGS) -o $@

$(LIBNAME): $(LIBOBJS)
	echo "[AR] $@"
	ar rcs $@ $^

-include ../../../base.mk

clean:
	echo "[RM] $^"
	-rm *.o *.d feedforward${EXT} $(LIBNAME)

.SILENT:
