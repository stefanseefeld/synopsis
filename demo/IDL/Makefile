synopsis := python synopsis.py

all: CosNaming Types Graphic Input html

html: tmp.syn
	$(synopsis) format --output=$@ $?
	touch $@

tmp.syn:	CosNaming.syn Types.syn Graphic.syn Input.syn
	$(synopsis) link --output=$@ $?

%.syn: %.idl
	$(synopsis) parse --output=$@ $?

%: %.syn
	$(synopsis) format --output=$@ $?
	touch $@

clean:
	rm -fr *.syn *~

distclean:	clean
	rm -rf CosNaming Types Graphic Input html
