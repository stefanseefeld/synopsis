# Compile Synopsis manual using docbook-utils (Jade or OpenJade wrappers)

SYNOPSIS	:= ../../bin/synopsis

XMLFILES	:= synopsis.xml

.PHONY:	all html pdf clean

all:	html pdf

html:	html/index.html

pdf:	synopsis-user.pdf

clean:
	rm -rf html *.pdf

config.xml:	../../Synopsis/Config.py
	$(SYNOPSIS) -p Python -Wp,-b,../../Synopsis/ -Wl,-p,javatags -f DocBook -Wf,-d,-m -o config.xml ../../Synopsis/Config.py.in

# The following rules use xsltproc (specifically the xmlto wrapper)

html/index.html:	$(XMLFILES)
	xmlto --skip-validation -o html -m synopsis-html.xsl html synopsis.xml && \
	    cp synopsis-html.css bg.png html/

#synopsis.pdf:	$(XMLFILES)
#	xmlto pdf synopsis.xml

## The following rules use Jade (specifically the docbook2 wrapper)
#
#html/index.html:	$(XMLFILES)
#	docbook2html -o html synopsis.xml
#
synopsis-user.pdf:	$(XMLFILES)
	docbook2pdf synopsis.xml && \
	mv synopsis.pdf synopsis-user.pdf
