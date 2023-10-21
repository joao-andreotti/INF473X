CFLAGS = -Og -ggdb -Wall -Wextra -lpthread

REBUILDABLES = client echo fancyclient

all: echo fancyclient

clean: 
	rm -f *.o $(REBUILDABLES)

echo: echo.c echo.h
	cc -o $@ $< $(CFLAGS)

fancyclient: fancyclient.c fancyclient.h
	cc -o $@ $< $(CFLAGS)