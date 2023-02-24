CC:=gcc
CFLAGS:=-O2 -Wall -Wextra -Wpedantic

all: hello-world international-hello-world

hello-world: hello-world.c
	$(CC) -o hello-world hello-world.c $(CFLAGS)

international-hello-world: international-hello-world.c
	$(CC) -o hello-world hello-world.c $(CFLAGS)

clean:
	rm -rf hello-world international-hello-world