PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -g -DXINERAMA # -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lm -lX11 -lXft -lfontconfig -lXinerama

SSRC= sarav2.c
SOBJ= ${SSRC:.c=.o}

all: sarav2 sarasock

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${SOBJ}: config.h

sarav2: ${SOBJ}
	${CC} -o $@ ${SOBJ} ${LIBS}

sarasock:
	gcc -c sarasock.c
	gcc -o sarasock sarasock.o

install: all
	install -Dm 755 sarav2 $(BINDIR)/sarav2
	install -Dm 755 sarasock $(BINDIR)/sarasock

uninstall:
	rm -f $(BINDIR)/sarav2
	rm -f $(BINDIR)/sarasock

clean:
	rm -f sarav2 sarasock *.o
