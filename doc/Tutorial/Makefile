srcdir	:= .

vpath Tutorial.xml $(srcdir)

all: html pdf

html: Tutorial.xml
	mkdir -p $@
	cp $(srcdir)/synopsis.css $@
	xsltproc --novalid --xinclude -o $@/ $(srcdir)/html.xsl $^
	mkdir -p $@/images
	cp $(srcdir)/images/*.png $@/images/

pdf: Tutorial.pdf

Tutorial.pdf: Tutorial.fo
	fop $^ $@

Tutorial.fo: Tutorial.xml
	xsltproc --novalid --xinclude -o $@ $(srcdir)/fo.xsl $^
	mkdir -p images
	cp -u $(srcdir)/images/*.svg images/

clean:
	rm Tutorial.fo

distclean: clean
	rm -rf html Tutorial.pdf

