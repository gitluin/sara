CFLAGS= -Wall -g -I/usr/include/freetype2 -Wno-deprecated-declarations
LIBS= -lX11 -lXft -lfontconfig
LDFLAGS=

SRC= sara.c
OBJ= ${SRC:.c=.o}

PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

all: sara

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h

sara: ${OBJ}
	${CC} ${LDFLAGS} -o $@ ${OBJ} ${LIBS}

install: all
	install -Dm 755 sara $(BINDIR)/sara

clean:
	rm -f sara *.o
