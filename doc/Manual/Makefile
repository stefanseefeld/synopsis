# Makefile for Synopsis RefManual

synopsis	:= ../../synopsis
bpath		:= ../..
spath		:= syn
cxxpath		:= ../../Parser/C++/occ/src/occ
cxxspath	:= syn/Parser/C++

F_HTML_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Formatter/HTML/*.py))
PYFILES		:= Formatter/ASCII Formatter/DUMP \
		   Core/AST Core/Type Core/Util \
		   Parser/IDL/omni \
		   Parser/Python/exparse Parser/Python/python \
		   $(F_HTML_PYFILES)

NOEX_PY		:= synopsis

PYSOURCES	:= $(patsubst %,$(bpath)/%.py,$(PYFILES)) \
		   $(patsubst %,$(bpath)/%,$(NOEX_PY))
PYSYNS		:= $(patsubst %,$(spath)/%.syn,$(PYFILES) $(NOEX_PY))

CXXFILES	:= synopsis builder type ast dict decoder swalker dumper

CXXSOURCES	:= $(patsubst %,$(cxxpath)/%.hh,$(CXXFILES))
CXXSYNS		:= $(patsubst %,$(cxxspath)/%.csyn,$(CXXFILES))

all:	manual

manual:   $(PYSYNS) $(CXXSYNS)
	$(synopsis) -Wl,-l -f HTML -Wf,--spec=html.spec -o Manual $^

# Compile Python files
$(spath)/%.syn:	$(bpath)/%.py
	@if [ ! -d $(spath) ]; then mkdir $(spath); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile main 'synopsis' file
$(spath)/synopsis.syn: $(bpath)/synopsis
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile C++ files
$(cxxspath)/%.csyn: $(cxxpath)/%.hh
	@if [ ! -d $(cxxpath) ]; then mkdir $(cxxpath); fi
	$(synopsis) -p C++ -Wp,-m,-b,$(bpath)/ -Wl,-p,ssd -o $@ $?

clean:
	rm -rf $(spath)

distclean:  clean
	rm -rf Manual
