#
# Makefile for libfireeagle.so
#
# Copyright (C) 2009 Yahoo! Inc
#
LIBOAUTHDIR := $(HOME)/scratch/liboauth
INCLUDE_DIRS := -I. -I../include -I$(LIBOAUTHDIR)/trunk/src
SRC_CC := ./fireeagle.cc ./fire_objects.cc ./fire_parser.cc
OBJS := $(SRC_CC:.cc=.o)
CPP := g++
CPPFLAGS := -fPIC -g -c $(INCLUDE_DIRS)
LD := g++
LDFLAGS := -fPIC -shared -L$(LIBOAUTHDIR)/trunk/src
RM := rm -f
TARGET := libfireeagle.so

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CPP) $(CPPFLAGS) -o $@ $<

clean:
	$(RM) $(OBJS) $(TARGET)
