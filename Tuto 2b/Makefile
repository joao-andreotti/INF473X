CFLAGS = -O2 -Wall -Wextra

REBUILDABLES = client echo fancyclient

all: client echo fancyclient

clean: 
	rm -f *.o $(REBUILDABLES)

client: client.c
	cc -o $@ $< $(CFLAGS)

echo: echo.c
	cc -o $@ $< $(CFLAGS)

fancyclient: fancyclient.c
	cc -o $@ $< $(CFLAGS)