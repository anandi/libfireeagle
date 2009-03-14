MAKE := make
DOXYGEN := doxygen
RMDIR := rm -Rf

all: src doc test

try:
	echo $(HEADERS)

clean:
	$(MAKE) -C test clean
	$(MAKE) -C src clean
	$(RMDIR) doc/
	$(RM) doc.d

src:
	$(MAKE) -C src

doc.d:
	echo -n "doc: " > $@
	find include/ -name *.h | xargs echo >> $@

doc:
	doxygen

test: src
	$(MAKE) -C test

include doc.d
