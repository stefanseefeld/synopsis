synopsis := python synopsis.py

all: CosNaming Types Graphic Input html

html: tmp.syn
	$(synopsis) format --output=$@ $?

tmp.syn:	CosNaming.syn Types.syn Graphic.syn Input.syn
	$(synopsis) link --output=$@ $?

%.syn: %.idl
	$(synopsis) parse --output=$@ $?

%: %.syn
	$(synopsis) format --output=$@ $?

clean:
	rm -fr *.syn *~

distclean:	clean
	rm -rf html
