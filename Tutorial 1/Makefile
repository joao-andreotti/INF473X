LANGUAGE=PORTUGUESE
CC:=gcc
CFLAGS:=-Og -ggdb -Wall -Wextra -Wpedantic

all: hello-world international-hello-world fancy-hello-world

hello-world: hello-world.c
	$(CC) -o hello-world hello-world.c $(CFLAGS)

international-hello-world: international-hello-world.c
	$(CC) -o international-hello-world international-hello-world.c $(CFLAGS) -D $(LANGUAGE)

fancy-hello-world: fancy-hello-world.c
	$(CC) -o fancy-hello-world fancy-hello-world.c $(CFLAGS)

package:
	zip pack *.c *.h Makefile README.md

clean:
	rm -rf hello-world international-hello-world fancy-hello-world pack.zip