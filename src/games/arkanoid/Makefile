3PPDIR=../../../3pp
INCLUDEDIR=../../../include
LIBNAME=libarkanoid.a
LIBOBJS=arkanoid.o
OBJS=$(LIBOBJS)

CCFLAGS = -g -Wall -std=c99 -I$(INCLUDEDIR)

all: $(LIBNAME)

$(LIBNAME): $(LIBOBJS)
	echo "[AR] $@"
	ar rcs $@ $^

-include ../../../base.mk

clean:
	echo "[RM] $^"
	-rm *.o *.d $(LIBNAME)

.SILENT:
