synopsis := python synopsis.py

all: html

html: AST.syn
	$(synopsis) format --output=$@ $?

AST.syn: AST.py
	$(synopsis) parse --output=$@ $?

clean:
	rm -fr *.syn *~

distclean:	clean
	rm -rf html