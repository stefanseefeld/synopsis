# Makefile for Synopsis RefManual

synopsis	:= ../../synopsis
bpath		:= ../../..
spath		:= syn
cxxpath		:= ../../../Synopsis/Parser/C++/occ/src/occ
cxxspath	:= syn/Parser/C++

F_HTML_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Formatter/HTML/*.py))
F_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Formatter/*.py))
C_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Core/*.py))
L_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Linker/*.py))
PYFILES		:= Synopsis/Parser/IDL/omni \
		   Synopsis/Parser/Python/exparse Synopsis/Parser/Python/python \
		   $(C_PYFILES) $(L_PYFILES) $(F_PYFILES) $(F_HTML_PYFILES)

NOEX_PY		:= Synopsis/synopsis

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
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile main 'synopsis' file
$(spath)/Synopsis/synopsis.syn: $(bpath)/Synopsis/synopsis
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile C++ files
$(cxxspath)/%.csyn: $(cxxpath)/%.hh
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p C++ -Wp,-m,-b,$(bpath)/ -Wl,-p,ssd -o $@ $?

clean:
	rm -rf $(spath)

distclean:  clean
	rm -rf Manual
