SHELL = /bin/sh

# compiler and flags
CC = gcc
CXX = g++
FLAGS = -Wall -O2
CFLAGS = $(FLAGS)
CXXFLAGS = $(CFLAGS)
LDFLAGS = -ludev

# build libraries and options
all: clean mediasmartserverd

clean:
	rm *.o mediasmartserverd core -f

device_monitor.o: src/device_monitor.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^

mediasmartserverd.o: src/mediasmartserverd.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^

# In case of multiarch
# mediasmartserverd: device_monitor.o mediasmartserverd.o /usr/lib/i386-linux-gnu/libudev.so
mediasmartserverd: device_monitor.o mediasmartserverd.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
