synopsis := python synopsis.py

all:	uml

uml:	Shapes.ps Shapes.html

Shapes.html:	main.syn
	$(synopsis) uml --title='Shapes' --format='html' --output=$@ $?

Shapes.ps:	main.syn
	$(synopsis) uml --title='Shapes' --format='ps' --output=$@ $?

main.syn: main.cc
	$(synopsis) parse --output=$@ $?

clean:
	rm -rf main.syn *~ 

distclean: clean
	rm -rf Shapes.ps