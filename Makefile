SHELL = /bin/bash

# compiler and flags
CC = gcc
CXX = g++
FLAGS = -Wall -O2
CFLAGS = $(FLAGS)
CXXFLAGS = $(CFLAGS)
LDFLAGS = -ludev

LIBUDEV := $(shell if ( [ "`lsb_release -is`" == "Ubuntu" ] && [ "`lsb_release -cs`" != "lucid" ] ); then echo "`uname -i`-linux-gnu/"; else echo ""; fi)

# build libraries and options
all: clean mediasmartserverd

clean:
	rm *.o mediasmartserverd core -f

device_monitor.o: src/device_monitor.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^

mediasmartserverd.o: src/mediasmartserverd.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^

mediasmartserverd: device_monitor.o mediasmartserverd.o /usr/lib/$(LIBUDEV)libudev.so
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
