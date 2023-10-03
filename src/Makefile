# Base Makefile for sdop.

# The normal targets in this Makefile are not called directly, but should be
# called from the top-level Makefile. To make it easy when developing from
# within the source directory, a call with no target gets passed back up to
# the outer Makefile, as does "make test".

intcall:;      cd ..; $(MAKE);
test:;         cd ..; $(MAKE) test;

# This is the main target

all:  buildhy hytest sdop svgtest

# Compile step for the modules

.SUFFIXES:  .o .c
.c.o:;  @echo "$(CC) $*.c"
	       $(FE)$(CC) -c $(CFLAGS) \
		 -DDATADIR='"$(DATADIR)"' \
		 -DSUPPORT_JPEG=$(SUPPORT_JPEG) \
		 -DSUPPORT_PNG=$(SUPPORT_PNG) \
		 $*.c

# List of general dependencies

DEP = ../Makefile Makefile functions.h globals.h mytypes.h sdop.h structs.h \
      ucp.h

# Object module list for sdop

SDOBJ = book.o datatables.o debug.o entity.o error.o font.o footnote.o \
        globals.o hyphen.o index.o jpeg.o misc.o number.o object.o page.o \
        para.o pin.o png.o preface.o read.o ref.o sdop.o svg.o sys.o \
        table.o toc.o tree.o ucd.o url.o utf8tables.o write.o

# Link steps for the programs

sdop:          $(SDOBJ)
	       @echo "$(CC) -o sdop"
	       $(FE)$(CC) $(CFLAGS) $(LDFLAGS) -o sdop $(SDOBJ) $(LIBS) -lm
	       @echo ">>> sdop command built"; echo ""

buildhy:       buildhy.o
	       @echo "$(CC) -o buildhy"
	       $(FE)$(CC) $(CFLAGS) $(LDFLAGS) -o buildhy buildhy.o
	       @echo ">>> buildhy command built"; echo ""

hytest:        hytest.o
	       @echo "$(CC) -o hytest"
	       $(FE)$(CC) $(CFLAGS) $(LDFLAGS) -o hytest hytest.o
	       @echo ">>> hytest command built"; echo ""

svgtest:       svgtest.o svg.o
	       @echo "$(CC) -o svgtest"
	       $(FE)$(CC) $(CFLAGS) $(LDFLAGS) -o svgtest svg.o svgtest.o -lm
	       @echo ">>> svgtest command built"; echo ""

# Dependencies

book.o:        $(DEP) book.c
buildhy.o:     $(DEP) buildhy.c
datatables.o:  $(DEP) datatables.c
debug.o:       $(DEP) debug.c
entity.o:      $(DEP) entity.c
error.o:       $(DEP) error.c
font.o:        $(DEP) font.c
footnote.o:    $(DEP) footnote.c
globals.o:     $(DEP) globals.c
hyphen.o:      $(DEP) hyphen.c
hytest.o:      $(DEP) hytest.c hyphen.c ucd.c utf8tables.c ucp.h
index.o:       $(DEP) index.c
jpeg.o:        $(DEP) jpeg.c
misc.o:        $(DEP) misc.c
number.o:      $(DEP) number.c
object.o:      $(DEP) object.c
page.o:        $(DEP) page.c
para.o:        $(DEP) para.c
pin.o:         $(DEP) pin.c
png.o:         $(DEP) png.c
preface.o:     $(DEP) preface.c
read.o:        $(DEP) read.c
ref.o:         $(DEP) ref.c
sdop.o:        $(DEP) sdop.c
svg.o:         $(DEP) svg.c nanosvg.h
svgtest.o:     $(DEP) svgtest.c svg.c nanosvg.h
sys.o:         $(DEP) sys.c
table.o:       $(DEP) table.c
toc.o:         $(DEP) toc.c
tree.o:        $(DEP) tree.c
ucd.o:         $(DEP) ucd.c
url.o:         $(DEP) url.c
utf8tables.o:  $(DEP) utf8tables.c
write.o:       $(DEP) write.c

# Clean up

clean:;       /bin/rm -f *.o sdop buildhy hytest svgtest

# End
