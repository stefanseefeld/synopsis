# Makefile for Synopsis RefManual

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

all:	manual

manual:   all.syn
	$(synopsis) -Wc,formatter=HTML -o Manual $^

manual-doxygen:   all.syn
	$(synopsis) -Wc,formatter=Doxygen -o ManualD $^

all.syn:	$(PYSYNS) $(CXXSYNS)
	$(synopsis) -Wc,linker=All -o all.syn $^

# Compile Python files
$(spath)/%.syn:	$(bpath)/%.py
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile main 'synopsis' file
$(spath)/Synopsis/synopsis.syn: $(bpath)/Synopsis/synopsis
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile C++ files
$(spath)/%.csyn: $(bpath)/%.hh
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(bpath)/%,syn/%-links,$?) -o $@ $?

clean:
	rm -rf $(spath)

distclean:  clean
	rm -rf Manual syn
