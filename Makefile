CC=gcc
CFLAGS=-Wall -g -O3
DEBUG_FLAGS=-fsanitize=address -Werror
LIBS= -lreadline -I/usr/include/SDL2 -D_REENTRANT -I/usr/include/libpng16 -I/usr/include/x86_64-linux-gnu -lSDL2_image -lSDL2

.PHONY: all clean

all: qr-openmp-empty

%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c -o $@ $<

sclimage: sclimage.o sclimage.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $< $(LIBS)

run:
	make sclimage
	./sclimage

clean:
	rm -f *.o sclimage
