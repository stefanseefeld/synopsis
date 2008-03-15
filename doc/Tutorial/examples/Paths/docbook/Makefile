prefix:= 
synopsis:= python synopsis.py
xsltproc:= xsltproc --novalid --path $(shell pwd) --xinclude
fop	:= xmlroff --backend=cairo
src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: Paths

Paths: Paths.xml Reference.xml
	mkdir -p $@
	cp synopsis.css $@
	$(xsltproc) -o $@/ html.xsl Reference.xml

Paths.pdf: Paths.fo
	$(fop) -o $@ $<

Paths.fo: Paths.xml Reference.xml
	$(xsltproc) -o $@ fo.xsl Reference.xml

Paths.xml: Paths.syn
	$(synopsis) docbook --title='The Title' --output=$@ $<

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
	cp Paths.xml $(prefix)
	if test -f Paths.pdf; then cp Paths.pdf $(prefix); fi
endif

install-print: Paths.pdf
ifdef prefix
	mkdir -p $(prefix)
	cp $< $(prefix)
endif

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths Paths.xml
