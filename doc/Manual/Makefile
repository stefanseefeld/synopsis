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
PYFILES		:= Synopsis/Parser/IDL/omni Synopsis/Config \
		   Synopsis/Parser/Python/exparse Synopsis/Parser/Python/python \
		   $(C_PYFILES) $(L_PYFILES) $(F_PYFILES) $(F_HTML_PYFILES)

NOEX_PY		:= Synopsis/synopsis

PYSOURCES	:= $(patsubst %,$(bpath)/%.py,$(PYFILES)) \
		   $(patsubst %,$(bpath)/%,$(NOEX_PY))
PYSYNS		:= $(patsubst %,$(spath)/%.syn,$(PYFILES) $(NOEX_PY))

CXXFILES	:= synopsis builder type ast dict decoder swalker dumper

CXXSOURCES	:= $(patsubst %,$(cxxpath)/%.hh,$(CXXFILES))
CXXSYNS		:= $(patsubst $(bpath)/%.hh,$(spath)/%.csyn,$(CXXSOURCES))

all:	manual

manual:   all.syn
	$(synopsis) -c config.py -Wc,formatter,HTML,style,synopsis -o Manual $^

all.syn:	$(PYSYNS) $(CXXSYNS)
	$(synopsis) -c config.py -Wl,-l -o all.syn $^

# Compile Python files
$(spath)/%.syn:	$(bpath)/%.py
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile main 'synopsis' file
$(spath)/Synopsis/synopsis.syn: $(bpath)/Synopsis/synopsis
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p Python -Wp,-b,$(bpath)/ -o $@ $?

# Compile C++ files
$(spath)/%.csyn: $(bpath)/%.hh
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -p C++ -Wp,-m,-b,$(bpath)/,-s,$(patsubst $(bpath)/%,syn/%-links,$?) -Wl,-p,ssd -o $@ $?

clean:
	rm -rf $(spath)

distclean:  clean
	rm -rf Manual
