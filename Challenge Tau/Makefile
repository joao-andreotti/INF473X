# Debug
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Og -ggdb
# Optimized
# CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -march=native -mtune=native -flto

LDFLAGS = -lpcap -ltins

LINK_TARGET = tcp_hijack
OBJS = main.o 

all: $(LINK_TARGET)

clean:
	rm -rf $(OBJS) $(LINK_TARGET)

pack:
	rm -rf submission.zip
	zip -r submission.zip *

$(LINK_TARGET): $(OBJS)
	g++ $^ -o $@ $(CXXFLAGS) $(LDFLAGS) 

%.o: %.cpp
	g++ -c $^ -o $@ $(CXXFLAGS) $(LDFLAGS)