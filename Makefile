PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
MANDIR?= ${PREFIX}/share/man
DOCDIR?= doc

CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L -DXINERAMA -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lX11 -lXft -lXinerama -lXext

SARASRC= sara.c util.c
SARAOBJ= ${SARASRC:.c=.o}

SOCKSRC= sarasock.c util.c
SOCKOBJ= ${SOCKSRC:.c=.o}

all: sara sarasock man

VPATH=src

config.h:
	cp config.def.h config.h

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${SARAOBJ}: config.h util.h types.h
${SOCKOBJ}: util.h

sara: ${SARAOBJ}
	${CC} -o $@ ${SARAOBJ} ${LIBS}

sarasock: ${SOCKOBJ}
	${CC} -o $@ ${SOCKOBJ} ${LIBS}

man: 
	install -Dm 644 $(DOCDIR)/sara.1 $(MANDIR)/man1

install: all
	install -Dsm 755 sara $(BINDIR)/sara
	install -Dsm 755 sarasock $(BINDIR)/sarasock

uninstall:
	rm -f $(BINDIR)/sara
	rm -f $(BINDIR)/sarasock
	rm -f $(MANDIR)/man1/sara.1

clean:
	rm -f sara sarasock *.o
