
CC=gcc
FLAGS=-shared

all: mute.c
	$(CC) $(FLAGS) mute.c -o mute.so 

install: mute.so
	cp mute.so /usr/lib/ladspa/mute.so

uninstall: /usr/lib/ladspa/mute.so
	rm -f /usr/lib/ladspa/mute.so

clean:
	rm -f mute.so
