#
# Makefile for libfireeagle.so
#
# Copyright (C) 2009 Yahoo! Inc
#
LIBOAUTHDIR := $(HOME)/gits/liboauth
INCLUDE_DIRS := -I. -I../include -I$(LIBOAUTHDIR)/trunk/src
SRC_CC := ./fireeagle.cc ./fire_objects.cc ./fireeagle_http.cc ./expat_parser.cc
OBJS := $(SRC_CC:.cc=.o)
DEPS := $(SRC_CC:.cc=.d)
CPP := g++
CPPFLAGS := -fPIC -g -c $(INCLUDE_DIRS)
LD := g++
LDFLAGS := -fPIC -shared -L$(LIBOAUTHDIR)/trunk/src
RM := rm -f

VERSION_MAJOR=$(shell grep MAJOR ../VERSION | cut -f 2 -d '=')
VERSION_MINOR=$(shell grep MINOR ../VERSION | cut -f 2 -d '=')
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR)

TARGET_BASE := libfireeagle.so
TARGET := $(TARGET_BASE).$(VERSION)
LINKS := $(TARGET_BASE).$(VERSION_MAJOR) $(TARGET_BASE)

INSTALL_BASE=/usr/local

all: build

build: $(TARGET) $(LINKS)

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CPP) $(CPPFLAGS) -o $@ $<

%.d: %.cc
	$(CPP) -o $@ -MM $(INCLUDE_DIRS) $<

$(LINKS): $(TARGET)
	for item in $@; do \
	    ln -s $< $$item; \
	done

include $(DEPS)

clean:
	$(RM) $(OBJS) $(TARGET) $(DEPS) $(LINKS)

install: $(TARGET)
	cp -r $(TARGET) $(INSTALL_BASE)/lib/
	for item in $(LINKS); do \
	    ln -s $(INSTALL_BASE)/lib/$(TARGET) $(INSTALL_BASE)/lib/$$item; \
	done
