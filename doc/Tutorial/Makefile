# Compile Synopsis manual using docbook-utils (Jade or OpenJade wrappers)

SYNOPSIS	:= ../../bin/synopsis

XMLFILES	:= synopsis.xml preface.xml intro.xml config.xml

.PHONY:	all html pdf clean

all:	html pdf

html:	index.html

pdf:	synopsis.pdf

clean:
	rm -f *.html *.pdf config.xml

config.xml:	../../Synopsis/Config.py
	$(SYNOPSIS) -p Python -Wp,-b,../../Synopsis/ -Wl,-p,javatags -f DocBook -Wf,-d,-m -o config.xml ../../Synopsis/Config.py.in

index.html:	$(XMLFILES)
	docbook2html synopsis.xml

synopsis.pdf:	$(XMLFILES)
	docbook2pdf synopsis.xml
