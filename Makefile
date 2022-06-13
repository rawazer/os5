CC=g++
CXX=g++

LIBSRC1=container.c
LIBSRC2=sockets.c

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

OBJECTS = container sockets

TAR=tar
TARFLAGS=-cvf
TARNAME=ex5.tar
TARSRCS= container.c sockets.c Makefile README

all: $(OBJECTS)

$(OBJECTS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	$(RM) $(OBJECTS) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
