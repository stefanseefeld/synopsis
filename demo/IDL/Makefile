synopsis := python synopsis.py

#-Wc,parser=IDL,linker=Linker,formatter=HTML

all: html

html: tmp.syn
	$(synopsis) format --output=$@ $?

tmp.syn:	CosNaming.syn Types.syn Graphic.syn Input.syn
	$(synopsis) link --output=$@ $?

%.syn: %.idl
	$(synopsis) parse --output=$@ $?

clean:
	rm -fr *.syn *~

distclean:	clean
	rm -rf html
