# Makefile for Synopsis RefManual

include ../../local.mk

topdir		:= ../..
synopsis	:= $(topdir)/synopsis -c config.py
sdir		:= syn
cxxdir		:= Synopsis/Parser/C++/occ/src/occ
cxxsdir		:= syn/Parser/C++

texi-help	:= aux cp fn fns ky log pg toc tp tps vr vrs

S_PYFILES	:= Synopsis/Config
F_HTML_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Formatter/HTML/*.py))
F_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Formatter/*.py))
C_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Core/*.py))
L_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Linker/*.py))
P_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Parser/*.py))
PP_PYFILES	:= $(patsubst $(topdir)/%.py, %, $(wildcard $(topdir)/Synopsis/Parser/*/*.py))
PYFILES		:= $(S_PYFILES) $(C_PYFILES) $(L_PYFILES) $(F_PYFILES) $(F_HTML_PYFILES) $(P_PYFILES) $(PP_PYFILES)
CXXHHFILES	:= $(patsubst %, $(cxxdir)/%, synopsis builder type ast dict decoder swalker dumper)
CXXCCFILES	:= $(patsubst %, $(cxxdir)/%, synopsis builder type ast dict decoder swalker dumper occ link)

PYSOURCES	:= $(patsubst %, $(topdir)/%.py, $(PYFILES))
CXXHHSOURCES	:= $(patsubst %, $(cxxdir)/%.hh, $(CXXHHFILES))
CXXCCSOURCES	:= $(patsubst %, $(cxxdir)/%.cc, $(CXXCCFILES))

S_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(S_PYFILES))
F_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(F_HTML_PYFILES) $(F_PYFILES))
C_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(C_PYFILES))
L_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(L_PYFILES))
P_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(P_PYFILES) $(PP_PYFILES))
PYSYNS		:= $(S_PYSYNS) $(F_PYSYNS) $(C_PYSYNS) $(L_PYSYNS) $(P_PYSYNS)
CXXHHSYNS	:= $(patsubst %, $(sdir)/%.csyn, $(CXXHHFILES))
CXXCCSYNS	:= $(patsubst %, $(sdir)/%.ccsyn, $(CXXCCFILES))

TEXI		:= core-ast.texi core-type.texi core-util.texi \
		   parser-c++.texi parser-idl.texi parser-py.texi \
		   linker.texi \
		   formatter-ascii.texi formatter-html.texi formatter-dump.texi formatter-dia.texi \
		   formatter-db.texi formatter-dot.texi formatter-html-simple.texi formatter-texi.texi

all:	html Manual.pdf

html:   all.syn
	$(synopsis) -Wc,formatter=HTML -o html $^

html-doxygen:   all.syn
	$(synopsis) -Wc,formatter=Doxygen -o html-doxygen $^

c++.syn:	$(CXXHHSYNS) $(CXXCCSYNS)
	$(synopsis) -Wc,linker=C++Final -o c++.syn $^

core-ast.syn:	$(S_PYSYNS) $(C_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::AST' -o $@ $^
core-type.syn:	$(S_PYSYNS) $(C_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::Type' -o $@ $^
core-util.syn:	$(S_PYSYNS) $(C_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::Util' -o $@ $^
parser-c++-py.syn:	$(P_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::C++' -o $@ $^
parser-c++-cpp.syn:	c++.syn
	$(synopsis) -Wc,linker=All -o $@ $^
parser-c++.syn:	parser-c++-py.syn parser-c++-cpp.syn
	$(synopsis) -Wc,linker=All -o $@ $^
parser-idl.syn:	$(P_PYSYNS) c++.syn
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::IDL' -o $@ $^
parser-py.syn:	$(P_PYSYNS) c++.syn
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::Python' -o $@ $^
linker.syn:	$(L_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Linker' -o $@ $^
formatter-ascii.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::ASCII' -o $@ $^
formatter-html.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML' -o $@ $^
formatter-dump.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::DUMP' -o $@ $^
formatter-dia.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dia' -o $@ $^
formatter-db.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::DocBook' -o $@ $^
formatter-dot.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dot' -o $@ $^
formatter-html-simple.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML_Simple' -o $@ $^
formatter-texi.syn:	$(F_PYSYNS)
	$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::TexInfo' -o $@ $^

%.texi:   %.syn
	$(synopsis) -Wc,formatter=TexInfo -o $@ $^

Manual.pdf:	$(TEXI) Manual.texi
		texi2pdf Manual.texi

all.syn:	$(PYSYNS) c++.syn
	$(synopsis) -Wc,linker=All -o all.syn $^

install: all
	mkdir -p $(datadir)/synopsis
	cp -pr html $(datadir)/synopsis
	install -m644 Manual.pdf $(datadir)/synopsis

clean:
	rm -rf $(sdir) *.pyc *.pyo *.syn $(TEXI) \
	$(addprefix Manual., $(texi-help))

distclean:  clean
	rm -rf html html-doxygen syn links.toc Manual.pdf

# Compile Python files
$(PYSYNS): $(sdir)/%.syn:	$(topdir)/%.py
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile main 'synopsis' file
$(sdir)/synopsis.syn: $(topdir)/synopsis
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile C++ .hh files
$(CXXHHSYNS): $(sdir)/%.csyn: $(topdir)/%.hh
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -I$(python_prefix)/include -DPYTHON_INCLUDE="$(python_include)" \
	-Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(topdir)/%,syn/%-links,$?) -o $@ $?

# Compile C++ .cc files
$(CXXCCSYNS): $(sdir)/%.ccsyn: $(topdir)/%.cc
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) -I$(topdir)/$(cxxdir)/.. -I$(python_prefix)/include -DPYTHON_INCLUDE="$(python_include)" \
	-Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(topdir)/%,syn/%-links,$?) -o $@ $?

# Create links (only) for C+ .cc files

