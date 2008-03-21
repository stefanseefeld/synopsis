prefix:= 
synopsis:= python synopsis.py
xsltproc:= xsltproc --novalid --path $(shell pwd) --xinclude
fop	:= xmlroff --backend=cairo
src	:= math.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: math

math: math.xml Reference.xml
	mkdir -p $@
	cp synopsis.css $@
	$(xsltproc) -o $@/ html.xsl Reference.xml

%.pdf: %.fo
	$(fop) -o $@ $<

math.fo: math.xml Reference.xml
	$(xsltproc) -o $@ fo.xsl Reference.xml

%.xml: %.syn
	$(synopsis) docbook --title='math reference' --output=$@ $<

%.syn: %.h
	$(synopsis) parse --output=$@ $<

install-html: all
ifdef prefix
	mkdir -p $(prefix)
	cp -r math/* $(prefix)
	if test -f math.pdf; then cp math.pdf $(prefix); fi
endif

install-print: math.pdf
ifdef prefix
	mkdir -p $(prefix)
	cp $< $(prefix)
endif

clean:
	rm -rf *.syn

distclean: clean
	rm -rf math math.xml
