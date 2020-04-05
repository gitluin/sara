PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

#CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -g -DXINERAMA #-D_POSIX_C_SOURCE=200809L -DXINERAMA -g -Os
CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L -DXINERAMA -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lm -lX11 -lXft -lXinerama

SRC= sara.c
OBJ= ${SRC:.c=.o}

all: sara sarasock

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${OBJ}: config.h

sara: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

sarasock:
	gcc -c sarasock.c
	gcc -o sarasock sarasock.o

install: all
	install -Dm 755 sara $(BINDIR)/sara
	install -Dm 755 sarasock $(BINDIR)/sarasock

uninstall:
	rm -f $(BINDIR)/sara
	rm -f $(BINDIR)/sarasock

clean:
	rm -f sara sarasock *.o
