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

LDFLAGS = -L. -L$(LIBDIR) -L$(3PPDIR)/pcg-c-0.94/src
ifeq ($(findstring CYGWIN,$(OSNAME)),CYGWIN)
# Used for this string: "CYGWIN_NT-10.0 DESKTOP-056Q0GE 2.5.2(0.297/5/3) 2016-06-23 14:29 x86_64 Cygwin"
	LDFLAGS += -ljobhandler_cyg
else ifeq ($(findstring Darwin,$(OSNAME)),Darwin)
	LDFLAGS += ../../libs/jobhandler/jobhandler.o
else
	LDFLAGS += -ljobhandler
endif
LDFLAGS += -lffann -lm -lpthread -lpcg_random -larkanoid

LIBBASEOBJS = arkanoid.o geometry.o
LIBOBJS = $(patsubst %.o,objs/%.o,$(LIBBASEOBJS))

TESTOBJS = $(patsubst %.o,objs/%.o,$(TESTBASEOBJS))

app: player$(EXT)

#addTrainer$(EXT) inspectNet$(EXT) 

player$(EXT): objs/player.o $(LIBDIR)/libffann.a objs/population.o
	@echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

addTrainer$(EXT): objs/addTrainer.o $(LIBDIR)/libffann.a objs/population.o
	@echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

inspectNet$(EXT): $(TESTOBJS) $(LIBNAME)
	@echo "[LD] $@"
	${GCC} $(CCFLAGS) $^ $(LDFLAGS) -o $@

$(LIBDIR)/libffann.a:
	make -C $(BASEDIR)/src/ai/feedforward libffann.a

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
	-rm objs/*.o objs/*.d $(LIBNAME)

#.SILENT:
