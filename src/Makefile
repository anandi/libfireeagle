#
# Makefile for libfireeagle.so
#
# Copyright (C) 2009 Yahoo! Inc
#
LIBOAUTHDIR := $(HOME)/scratch/liboauth
INCLUDE_DIRS := -I. -I../include -I$(LIBOAUTHDIR)/trunk/src
SRC_CC := ./fireeagle.cc ./fire_objects.cc ./fireeagle_http.cc ./expat_parser.cc
OBJS := $(SRC_CC:.cc=.o)
DEPS := $(SRC_CC:.cc=.d)
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

%.d: %.cc
	$(CPP) -o $@ -MM $(INCLUDE_DIRS) $<

include $(DEPS)

clean:
	$(RM) $(OBJS) $(TARGET) $(DEPS)
