# Makefile for Synopsis RefManual

include ../../local.mk

topdir		:= ../..
synopsis	:= $(topdir)/synopsis -c config.py
sdir		:= syn
cxxdir		:= Synopsis/Parser/C++
cxxsdir		:= syn/Parser/C++

texi-help	:= aux cp cps fn fns ky log pg toc tp tps vr vrs

PYSOURCES	:= $(wildcard $(topdir)/Synopsis/*.py)
PYSOURCES	:= $(PYSOURCES) $(wildcard $(topdir)/Synopsis/*/*.py)
PYSOURCES	:= $(PYSOURCES) $(wildcard $(topdir)/Synopsis/*/*/*.py)
#PY_HTMLSOURCES	:= $(wildcard $(topdir)/Synopsis/Formatter/HTML/*.py)
#PY_FSOURCES	:= $(wildcard $(topdir)/Synopsis/Formatter/*.py)
#PY_CSOURCES	:= $(wildcard $(topdir)/Synopsis/Core/*.py)
#PY_LSOURCES	:= $(wildcard $(topdir)/Synopsis/Linker/*.py)
#PY_PSOURCES	:= $(wildcard $(topdir)/Synopsis/Parser/*.py)
#PY_PPSOURCES	:= $(wildcard $(topdir)/Synopsis/Parser/*/*.py)
#PYSOURCES	:= $(PY_SSOURCES) $(PY_CSOURCES) $(PY_LSOURCES) $(PY_FSOURCES) $(PY_HTMLSOURCES) $(PY_PSOURCES) $(PY_PPSOURCES)
CXXCCSOURCES    := $(wildcard $(topdir)/Synopsis/Parser/C++/syn/*.cc)
#CXXCCSOURCES    := $(wildcard $(topdir)/Synopsis/Parser/C++/occ/*.cc) $(CXXCCSOURCES)
CXXHHSOURCES    := $(wildcard $(topdir)/Synopsis/Parser/C++/syn/*.hh)
#CXXHHSOURCES    := $(wildcard $(topdir)/Synopsis/Parser/C++/occ/*.hh) $(CXXHHSOURCES)


#S_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(S_PYFILES))
#F_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(F_HTML_PYFILES) $(F_PYFILES))
#C_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(C_PYFILES))
#L_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(L_PYFILES))
#P_PYSYNS	:= $(patsubst %, $(sdir)/%.syn, $(P_PYFILES) $(PP_PYFILES))
#PYSYNS		:= $(S_PYSYNS) $(F_PYSYNS) $(C_PYSYNS) $(L_PYSYNS) $(P_PYSYNS)
PYSYNS		:= $(patsubst $(topdir)/%.py, $(sdir)/%.syn, $(PYSOURCES))
CXXHHSYNS	:= $(patsubst $(topdir)/%.hh, $(sdir)/%.hh.syn, $(CXXHHSOURCES))
CXXCCSYNS	:= $(patsubst $(topdir)/%.cc, $(sdir)/%.cc.syn, $(CXXCCSOURCES))

TEXI		:= core-ast.texi core-type.texi core-util.texi \
		   parser-c++.texi parser-idl.texi parser-py.texi \
		   linker.texi \
		   formatter-ascii.texi formatter-html.texi formatter-dump.texi formatter-dia.texi \
		   formatter-db.texi formatter-dot.texi formatter-html-simple.texi formatter-texi.texi

all:	html info Manual.pdf

html:   all.syn
	@echo formatting $@
	@$(synopsis) -Wc,formatter=HTML -o html $^

html-doxygen:   all.syn
	@echo formatting $@
	@$(synopsis) -Wc,formatter=Doxygen -o html-doxygen $^

c++.syn:	$(CXXHHSYNS) $(CXXCCSYNS)
	@echo linking C++ files together
	@$(synopsis) -Wc,linker=C++Final -o c++.syn $^

py.syn:	$(PYSYNS)
	@echo linking Python files together
	@$(synopsis) -o py.syn $^

core-ast.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::AST' -o $@ $^
core-type.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::Type' -o $@ $^
core-util.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Core::Util' -o $@ $^
parser-c++-py.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::C++' -o $@ $^
parser-c++-cpp.syn:	c++.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -o $@ $^
parser-c++.syn:	parser-c++-py.syn parser-c++-cpp.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -o $@ $^
parser-idl.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::IDL' -o $@ $^
parser-py.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Parser::Python' -o $@ $^
linker.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Linker' -o $@ $^
formatter-ascii.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::ASCII' -o $@ $^
formatter-html.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML' -o $@ $^
formatter-dump.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::DUMP' -o $@ $^
formatter-dia.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dia' -o $@ $^
formatter-db.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::DocBook' -o $@ $^
formatter-dot.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dot' -o $@ $^
formatter-html-simple.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML_Simple' -o $@ $^
formatter-texi.syn:	py.syn
	@echo linking $@
	@$(synopsis) -Wc,linker=All -Wl,-s,'Synopsis::Formatter::TexInfo' -o $@ $^

%.texi:   %.syn
	@echo formatting $@
	@$(synopsis) -Wc,formatter=TexInfo -o $@ $^

Manual.pdf:	$(TEXI) Manual.texi
	@echo generating $@
	texi2pdf Manual.texi

info:	$(TEXI) Manual.texi
	@echo generating info files
	makeinfo Manual.texi

all.syn:	py.syn c++.syn
	@echo linking all files together
	@$(synopsis) -Wc,linker=All -o all.syn $^

install: all
	mkdir -p $(datadir)/synopsis
	cp -pr html $(datadir)/synopsis
	mkdir -p $(infodir)
	install -m644 synopsis.info* $(infodir)
	install -m644 Manual.pdf $(datadir)/synopsis

clean:
	rm -rf $(sdir) *.pyc *.pyo *.syn $(TEXI) \
	$(addprefix Manual., $(texi-help))

distclean:  clean
	rm -rf html html-doxygen syn links.toc Manual.pdf *.info* 

# Compile Python files
$(PYSYNS): $(sdir)/%.syn:	$(topdir)/%.py
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile main 'synopsis' file
$(sdir)/synopsis.syn: $(topdir)/synopsis
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@$(synopsis) -Wc,parser=Py,linker=Py -o $@ $?

# Compile C++ .hh files
$(CXXHHSYNS): $(sdir)/%.hh.syn: $(topdir)/%.hh
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@$(synopsis) -I$(topdir)/$(cxxdir) -DPYTHON_INCLUDE="$(python_include)" \
	-Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(topdir)/%,syn/%-links,$?) -o $@ $?

# Compile C++ .cc files
$(CXXCCSYNS): $(sdir)/%.cc.syn: $(topdir)/%.cc
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	@$(synopsis) -I$(topdir)/$(cxxdir) -DPYTHON_INCLUDE="$(python_include)" \
	-Wc,parser=C++,linker=C++ -Wp,-s,$(patsubst $(topdir)/%,syn/%-links,$?) -o $@ $?

# Create links (only) for C+ .cc files

