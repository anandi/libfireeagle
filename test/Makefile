#
# Makefile for deskapp
#
# Copyright (C) 2009 Yahoo! Inc
#
LIBOAUTHDIR := $(HOME)/scratch/liboauth
INCLUDE_DIRS := -I. -I../include
LIBDIRS := -L../src -L$(LIBOAUTHDIR)/trunk/src
LIBS := -loauth -lfireeagle -lcurl -lexpat
SRC_CC := ./deskapp.cc
OBJS := $(SRC_CC:.cc=.o)
DEPS := $(SRC_CC:.cc=.d)
CPP := g++
CPPFLAGS := -g -c $(INCLUDE_DIRS)
LD := g++
LDFLAGS := $(LIBDIRS) $(LIBS)
RM := rm -f
TARGET := deskapp

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
