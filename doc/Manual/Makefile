# Makefile for Synopsis RefManual

synopsis = ../../synopsis
bpath = ../..
spath = syn

PYFILES = Formatter/HTML Formatter/ASCII Formatter/DUMP \
	Synopsis/AST Synopsis/Type Synopsis/Util \
	Parser/IDL/omni \
	Parser/Python/exparse Parser/Python/python

PYSOURCES = $(patsubst %,$(bpath)/%.py,$(PYFILES))
PYSYNS = $(patsubst %,$(spath)/%.syn,$(PYFILES))


all:	manual

manual:	$(PYSYNS)
	$(synopsis) -f HTML -Wf,-s,style.css -o Manual $^

$(spath)/%.syn:	$(bpath)/%.py
	@if [ ! -d $(spath) ]; then mkdir $(spath); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

clean:
	rm -rf $(spath)

distclean:  clean
	rm -rf Manual
