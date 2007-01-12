
CC=gcc
CFLAGS=-O3 -g

NBIT=32
ifeq ($(NBIT),32)
ifeq ($(findstring x86_64,$(shell uname -m)),x86_64)
	LIBS=-m32 -L/emul/linux/x86/usr/lib
	override CFLAGS+= -m32
endif
endif

OBJS=layer1.o layer2.o layer3.o synth.o tables.o mpadec.o mp3dec.o
SOOBJS=$(OBJS:.o=.lo)

libmpadec.a: $(OBJS)
	ar cr libmpadec.a $(OBJS)
	ranlib libmpadec.a

libmpadec.so: $(SOOBJS)
	$(CC) -shared -fPIC -o $@ $^ $(LIBS)

mpadec.dll: $(OBJS)
	dllwrap -mwindows --dllname $@ --def $(@:dll=def) $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.lo: %.c
	$(CC) -fPIC $(CFLAGS) -c $< -o $@

clean:
	-rm -f $(OBJS) libmpadec.a
	-rm -f libmpadec.so $(SOOBJS)
	-rm -f mpadec.dll
