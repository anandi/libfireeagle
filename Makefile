#
# Makefile for libfireeagle.so
#
# Copyright (C) 2009 Yahoo! Inc
#
MAKE := make
DOXYGEN := doxygen
RMDIR := rm -Rf

SUBDIRS = src test

INSTALL_BASE = /usr/local

VERSION_MAJOR=$(shell grep MAJOR VERSION | cut -f 2 -d '=')
VERSION_MINOR=$(shell grep MINOR VERSION | cut -f 2 -d '=')
VERSION=$(VERSION_MAJOR).$(VERSION_MINOR)

.PHONY: subdirs $(SUBDIRS)

all: build

build: subdirs doc

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(MAKE) -C test clean
	$(MAKE) -C src clean
	$(RMDIR) doc/
	$(RM) doc.d

doc.d:
	echo -n "doc: " > $@
	find include/ -name *.h | xargs echo >> $@

doc:
	doxygen

test: src

install: build
	$(MAKE) -C src install
	mkdir -p $(INSTALL_BASE)/include/fireeagle
	cp -r include/* $(INSTALL_BASE)/include/fireeagle/
	mkdir -p /usr/share/docs/libfireeagle-$(VERSION)
	cp -r doc/* /usr/share/docs/libfireeagle-$(VERSION)

include doc.d
