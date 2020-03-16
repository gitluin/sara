PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

#CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -g -DXINERAMA # -Os
CFLAGS= -std=c99 -Wall -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L -DXINERAMA -Os
INCFLAGS= -I/usr/include/freetype2
LIBS= -lm -lX11 -lXft -lXinerama

SRC= saranobar.c
OBJ= ${SRC:.c=.o}

all: saranobar sarasock

.c.o:
	${CC} -c ${CFLAGS} ${INCFLAGS} $<

${OBJ}: config.h

saranobar: ${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS}

sarasock:
	gcc -c sarasock.c
	gcc -o sarasock sarasock.o

install: all
	install -Dm 755 saranobar $(BINDIR)/saranobar
	install -Dm 755 sarasock $(BINDIR)/sarasock

uninstall:
	rm -f $(BINDIR)/saranobar
	rm -f $(BINDIR)/sarasock

clean:
	rm -f saranobar sarasock *.o
