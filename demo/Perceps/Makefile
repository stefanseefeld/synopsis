synopsis := python synopsis.py

all: Manual ManualD

Manual: Manual.syn
	$(synopsis) html --output=$@ $?

ManualD: Manual.syn
	$(synopsis) doxygen --output=$@ $?

Manual.syn: display.syn
	$(synopsis) link --output=$@ $?

display.syn: display.h
	$(synopsis) parse --output=$@ $?

clean:
	rm -rf *.syn *~

distclean: clean
	rm -rf Manual ManualD
