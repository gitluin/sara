PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
MANDIR?= ${PREFIX}/share/man

#CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -g -DXINERAMA #-D_POSIX_C_SOURCE=200809L -DXINERAMA -g -Os
CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L -DXINERAMA -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lX11 -lXft -lXinerama

SRC= sara.c
OBJ= ${SRC:.c=.o}

all: sara sarasock man

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${OBJ}: config.h

sara: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

sarasock:
	${CC} -c sarasock.c
	${CC} -o sarasock sarasock.o

man: 
	install -Dm 644 sara.1 $(MANDIR)/man1

install: all
	install -Dsm 755 sara $(BINDIR)/sara
	install -Dsm 755 sarasock $(BINDIR)/sarasock

uninstall:
	rm -f $(BINDIR)/sara
	rm -f $(BINDIR)/sarasock
	rm -f $(MANDIR)/man1/sara.1

clean:
	rm -f sara sarasock *.o
