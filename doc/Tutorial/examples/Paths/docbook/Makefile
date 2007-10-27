prefix:= 
synopsis:= python synopsis.py

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: Paths

Paths: Paths.xml
	mkdir -p $@
	cp synopsis.css $@
	xsltproc --novalid --path $(shell pwd) --xinclude -o $@/ html.xsl $<

Paths.xml: Paths.syn
	$(synopsis) docbook --output=$@ $<

Paths.syn: $(syn)
	$(synopsis) link --output=$@ $^

Path.syn: ../src/Path.h
	$(synopsis) cxx_ssd_prev --output=$@ $<

Polyline.syn: ../src/Polyline.h
	$(synopsis) cxx_ss --output=$@ $<

Bezier.syn: ../src/Bezier.h
	$(synopsis) cxx_javadoc --output=$@ $<

Nurbs.syn: ../src/Nurbs.h
	$(synopsis) cxx_rst --output=$@ $<

install-html: all
ifdef prefix
	mkdir -p $(prefix)
	cp -r Paths/* $(prefix)
endif

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths Paths.xml
