CXXFLAGS=-Wall -std=c++17 -O3 -fno-exceptions

.PHONY: all clean

all: sips

sips:
	$(CXX) $(CXXFLAGS) -o $@ sips.cpp

clean:
	rm sips || true
