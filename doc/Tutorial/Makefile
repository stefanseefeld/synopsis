srcdir	:= .
processors := Synopsis.Parsers.Python.Parser \
              Synopsis.Parsers.IDL.Parser \
              Synopsis.Parsers.Cxx.Parser \
              Synopsis.Processors.Linker \
              Synopsis.Formatters.Dump.Formatter \
              Synopsis.Formatters.Docbook.Formatter \
              Synopsis.Formatters.Texinfo.Formatter \
              Synopsis.Formatters.HTML.Formatter

appendix	:= $(patsubst %, %.xml, $(processors))

vpath Tutorial.xml $(srcdir)

all: html pdf

html: Tutorial.xml $(appendix)
	mkdir -p $@
	cp $(srcdir)/synopsis.css $@
	xsltproc --novalid --path $(shell pwd) --xinclude -o $@/ $(srcdir)/html.xsl $<
	mkdir -p $@/images
	cp $(srcdir)/images/*.png $@/images/

pdf: Tutorial.pdf

Tutorial.pdf: Tutorial.fo $(appendix)
	fop $< $@

Tutorial.fo: Tutorial.xml
	xsltproc --novalid --xinclude -o $@ $(srcdir)/fo.xsl $^
	mkdir -p images
	cp -u $(srcdir)/images/*.svg images/

$(appendix): %.xml:
	python $(srcdir)/parameters.py $(patsubst %.xml, %, $@) $@

clean:
	rm Tutorial.fo

distclean: clean
	rm -rf html Tutorial.pdf

