srcdir	:= .

vpath Tutorial.xml $(srcdir)

all: html pdf

html: Tutorial.xml
	mkdir -p $@
	cp $(srcdir)/synopsis.css $@
	xsltproc --novalid -o $@/ $(srcdir)/html.xsl $^
	cp -r $(srcdir)/images $@

pdf: Tutorial.pdf

Tutorial.pdf: Tutorial.xml
	docbook2pdf $^

clean:

distclean: clean
	rm -rf html Tutorial.pdf

