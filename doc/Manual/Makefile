# Makefile for Synopsis RefManual

include ../../local.mk

synopsis	:= ../../synopsis -c config.py
bpath		:= ../../..
spath		:= syn
cxxpath		:= ../../../Synopsis/Parser/C++/occ/src/occ
cxxspath	:= syn/Parser/C++

F_HTML_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Formatter/HTML/*.py))
F_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Formatter/*.py))
C_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Core/*.py))
L_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Linker/*.py))
P_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Parser/*.py))
PP_PYFILES	:= $(patsubst $(bpath)/%.py,%,$(wildcard $(bpath)/Synopsis/Parser/*/*.py))
PYFILES		:= Synopsis/Config \
		   $(C_PYFILES) $(L_PYFILES) $(F_PYFILES) $(F_HTML_PYFILES) $(P_PYFILES) $(PP_PYFILES)

NOEX_PY		:= Synopsis/synopsis

PYSOURCES	:= $(patsubst %,$(bpath)/%.py,$(PYFILES)) \
		   $(patsubst %,$(bpath)/%,$(NOEX_PY))
PYSYNS		:= $(patsubst %,$(spath)/%.syn,$(PYFILES) $(NOEX_PY))

CXXFILES	:= synopsis builder type ast dict decoder swalker dumper

CXXSOURCES	:= $(patsubst %,$(cxxpath)/%.hh,$(CXXFILES))
CXXSYNS		:= $(patsubst $(bpath)/%.hh,$(spath)/%.csyn,$(CXXSOURCES))

CXXCCFILES	:= $(CXXFILES) occ link
CXXCCSOURCES	:= $(patsubst %,$(cxxpath)/%.cc,$(CXXCCFILES))
CXXCCSYNS	:= $(patsubst $(bpath)/%.cc,$(spath)/%.ccsyn,$(CXXCCSOURCES))

all:	manual

manual:   all.syn
	$(synopsis) -Wc,formatter=HTML -o Manual $^

manual-doxygen:   all.syn
	$(synopsis) -Wc,formatter=Doxygen -o ManualD $^

all.syn:	$(PYSYNS) c++.syn
	$(synopsis) -Wc,linker=All -o all.syn $^

c++.syn:	$(CXXSYNS) $(CXXCCSYNS)
	$(synopsis) -Wc,linker=C++Final -o c++.syn $^

# Compile Python files
$(spath)/%.syn:	$(bpath)/%.py
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile main 'synopsis' file
$(spath)/Synopsis/synopsis.syn: $(bpath)/Synopsis/synopsis
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile C++ .hh files
$(spath)/%.csyn: $(bpath)/%.hh
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -I$(python_prefix)/include -DPYTHON_INCLUDE="$(python_include)" -Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(bpath)/%,syn/%-links,$?) -o $@ $?

# Compile C++ .cc files
$(spath)/%.ccsyn: $(bpath)/%.cc
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -I$(python_prefix)/include -DPYTHON_INCLUDE="$(python_include)" -Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(bpath)/%,syn/%-links,$?),-I$(cxxpath)/.. -o $@ $?

# Create links (only) for C+ .cc files

clean:
	rm -rf $(spath) *.pyc *.pyo

distclean:  clean
	rm -rf Manual syn links.toc
