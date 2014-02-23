PROG    = lightpad

CC      = gcc
LIBS    = `pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0`
CFLAGS  = -std=c99 -Wall -Wextra

PREFIX   ?= /usr/local
BINPREFIX = $(PREFIX)/bin

SRC = src/${PROG}.c src/io.c src/editor.c
OBJ = $(SRC:.c=.o)

all: CFLAGS += -Os
all: $(PROG)

debug: CFLAGS += -O0 -g -pedantic -DDEBUG
debug: all

.c.o:
	$(CC) $(LIBS) $(CFLAGS) -c -o $@ $<

$(PROG): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

install:
	mkdir -p $(DESTDIR)$(BINPREFIX)
	mkdir -p $(DESTDIR)/usr/share/applications/
	install -m 0755 src/$(PROG) $(DESTDIR)/$(BINPREFIX)/
	install -m 0644 data/$(PROG).desktop $(DESTDIR)/usr/share/applications/

uninstall:
	rm -f $(BINPREFIX)/$(PROG)
	rm -f /usr/share/applications/$(PROG).desktop

clean:
	rm -f $(OBJ) src/$(PROG)

.PHONY: all debug clean install uninstall
