synopsis:= python synopsis.py

all:
	@echo "The boost demo is not run by default. If you have the boost sources,"
	@echo "you must create a symlink to the boost root directory as 'boost' here"
	@echo "eg: ln -s ~/boost boost"
	@echo "Then you can run 'make bpl'"

bpl: bpl.syn bpl.xref
	$(synopsis) html --output=$@ $<

sxr: bpl.syn
	$(synopsis) sxr --url=http://localhost:8000/sxr.cgi --output=$@ $<
	@echo "now start 'sxr-server -d sxr -u /sxr.cgi -s /files' to activate the sxr engine"

bpl.syn: boost/boost/python.hpp
	$(synopsis) parse --output=$@ $?

bpl.xref: bpl.syn
	$(synopsis) xref --output=$@ $<

clean:
	rm -rf *~ xref links bpl.syn bpl.xref

distclean:	clean
	rm -rf bpl sxr
