synopsis := python synopsis.py

all:
		@echo "The boost demo is not run by default. If you have the boost sources,"
		@echo "you must create a symlink to the boost root directory as 'boost' here"
		@echo "eg: ln -s ~/boost boost"
		@echo "Then you can run 'make bpl'"

bpl: bpl.syn bpl.xref
	$(synopsis) format --output=$@ $<

bpl.syn xref/boost/python.hpp links/boost/python.hpp: boost/boost/python.hpp
	$(synopsis) parse --output=$@ $?

bpl.xref: bpl.syn xref/boost/python.hpp
	$(synopsis) xref --output=$@ $<


clean:
	rm -rf *~ xref links bpl.syn bpl.xref

distclean:	clean
	rm -rf bpl
