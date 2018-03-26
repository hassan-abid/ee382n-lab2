IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR) -g -std=gnu99

ODIR=obj
LDIR =./lib

LIBS=-lm

_DEPS = MemoryAccess.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = MemoryAccess.o int_latency.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

Lab2: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
