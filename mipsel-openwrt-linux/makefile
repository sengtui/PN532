##
## LINUX barebone makefile for c++ examples : good for all platforms
##
## Simply run make or make clean
##
## Intend this makefile only as a "batch examples updater" after library modification.
##

SHELL	:= /bin/bash
Libs     := -lmraa 
CXX      := mipsel-openwrt-linux-g++
CC       := mipsel-openwrt-linux-gcc
CXXFLAGS :=-O3
CFLAGS   :=
.PHONY: all clean

%.o: ../%.cpp
	$(TOOLCHAIN_DIR)/bin/$(CXX) $(CXXFLAGS) -c $<


test: pn532.o nfc_test.o
	$(TOOLCHAIN_DIR)/bin/$(CXX) $(CXXFLAGS) -o test pn532.o nfc_test.o $(Libs)


all: test

clean:
	$(RM) test pn532.o nfc_test.o
