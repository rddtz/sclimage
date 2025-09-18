CC=gcc
CFLAGS=-Wall -g -O3
DEBUG_FLAGS=-fsanitize=address -Werror
LIBS= -lreadline -I/usr/include/SDL2 -I/usr/include/libpng16 -I/usr/include/x86_64-linux-gnu -D_REENTRANT -I/usr/include/harfbuzz -I/usr/include/freetype2 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -lSDL2_image -lSDL2_ttf -lSDL2

.PHONY: all clean

all: sclimage

%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

sclimage: src/sclimage.o src/sclimage.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $< $(LIBS)

run:
	make sclimage
	./sclimage

clean:
	rm -f *.o src/*.o sclimage
