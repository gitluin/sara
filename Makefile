PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -g # -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lm -lX11 -lXft -lfontconfig

SRC= sara.c
OBJ= ${SRC:.c=.o}

all: sara

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${OBJ}: config.h

sara: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

install: all
	install -Dm 755 sara $(BINDIR)/sara

clean:
	rm -f sara *.o
