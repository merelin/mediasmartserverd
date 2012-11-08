SHELL = /bin/bash

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

mediasmartserverd: device_monitor.o mediasmartserverd.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

prepare-for-packaging:
	@if [ "$(PACKAGE_VERSION)" != "" ]; then \
		cd ..; \
		mkdir mediasmartserver; \
		cp -RLv mediasmartserverd/{LICENSE,Makefile,readme.txt,README.md,src,debian,etc} mediasmartserver; \
		tar cfz mediasmartserver-$(PACKAGE_VERSION).tar.gz mediasmartserver; \
		rm -rf mediasmartserver; \
		bzr dh-make mediasmartserver $(PACKAGE_VERSION) mediasmartserver-$(PACKAGE_VERSION).tar.gz; \
		rm -rf mediasmartserver/debian/{*.ex,*.EX,README.Debian,README.source}; \
		cd mediasmartserver; \
		bzr commit -m "Initial commit of Debian packaging."; \
	fi

package-unsigned: prepare-for-packaging
	@if [ "$(PACKAGE_VERSION)" != "" ]; then \
		cd ../mediasmartserver; \
		bzr builddeb -- -us -uc; \
	fi

package-signed: prepare-for-packaging
	@if [ "$(PACKAGE_VERSION)" != "" ]; then \
		cd ../mediasmartserver; \
		bzr builddeb -S; \
	fi

