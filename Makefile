#
# Makefile for libfireeagle.so
#
# Copyright (C) 2009 Yahoo! Inc
#
MAKE := make
DOXYGEN := doxygen
RMDIR := rm -Rf

SUBDIRS = src test

.PHONY: subdirs $(SUBDIRS)

all: subdirs doc

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

include doc.d
