BASEDIR:=../../..
MODULEDIR:=$(CURDIR)
INCLUDEDIR:=$(BASEDIR)/include
LIBDIR:=$(BASEDIR)/libs
3PPDIR:=$(BASEDIR)/3pp

GCC=gcc

OSNAME:=$(shell uname -s)
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
  EXT:=.exe
else
  EXT:=
endif

CCFLAGS = -g -Wall \
	-std=c99 \
	-I$(3PPDIR)/pcg-c-0.94/include \
	-I$(LIBDIR) -I$(INCLUDEDIR) -Iinclude

LDFLAGS = -L$(LIBDIR) -L.
LDFLAGS += -lffann -lm -lpthread -L$(3PPDIR)/pcg-c-0.94/src -lpcg_random

LIBNAME=$(LIBDIR)/libffann.a

LIBBASEOBJS = activation.o neurons.o layer.o network.o
BASEOBJS = $(LIBBASEOBJS) main.o
OBJS = $(patsubst %.o,objs/%.o,$(BASEOBJS))
LIBOBJS = $(patsubst %.o,objs/%.o,$(LIBBASEOBJS))

all: feedforward$(EXT)

feedforward$(EXT): $(OBJS) $(LIBNAME)
	@echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

$(LIBNAME): $(LIBOBJS)
	echo "[AR] $@"
	ar rcs $@ $^

-include $(OBJS:.o=.d)

# compile and generate dependency info;
# more complicated dependency computation, so all prereqs listed
# will also become command-less, prereq-less targets
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
objs/%.o: %.c
	echo "[CC] $@"
	mkdir -p $(MODULEDIR)/objs/
	$(GCC) -c $(CCFLAGS) $*.c -o $@
	$(GCC) -MM $(CCFLAGS) $*.c > objs/$*.d
	mv -f objs/$*.d objs/$*.d.tmp
	sed -e 's|.*:|$@:|' < objs/$*.d.tmp > objs/$*.d
	cp -f objs/$*.d objs/$*.d.tmp
	sed -e 's/.*://' -e 's/\\$$//' < objs/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> objs/$*.d
	rm -f objs/$*.d.tmp

clean:
	echo "[RM] $^"
	-rm objs/*.o objs/*.d feedforward${EXT}

.SILENT:
