synopsis := python synopsis.py

topdir	:= ../..

py_src		:= $(shell find $(topdir)/Synopsis -name '*.py' -print)
py_syn		:= $(patsubst $(topdir)/%.py, %.syn, $(py_src))
cxx_hh  	:= $(wildcard $(topdir)/Synopsis/Parsers/Cxx/syn/*.hh)
cxx_hh_syn	:= $(patsubst $(topdir)/%.hh, %.hh.syn, $(cxx_hh))
cxx_h		:= $(wildcard $(topdir)/Synopsis/Parsers/Cxx/occ/*.h)
cxx_h_syn	:= $(patsubst $(topdir)/%.h, %.h.syn, $(cxx_h))
cxx_cc		:= $(wildcard $(topdir)/Synopsis/Parser/C++/syn/*.cc)
cxx_cc		+= $(wildcard $(topdir)/Synopsis/Parser/C++/occ/*.cc)

TEXI		:= core-ast.texi core-type.texi core-util.texi \
		   parser-c++.texi parser-idl.texi parser-py.texi \
		   linker.texi \
		   formatter-ascii.texi formatter-html.texi formatter-dump.texi formatter-dia.texi \
		   formatter-db.texi formatter-dot.texi formatter-html-simple.texi formatter-texi.texi


all: py.syn

vpath %.py $(topdir)
vpath %.cc $(topdir)
vpath %.h $(topdir)
vpath %.hh $(topdir)

html:   c++.syn
	@echo formatting $@
	@$(synopsis) html --output=$@ $^

# Compile Python files
$(py_syn): %.syn: %.py
	@echo parsing $^
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) python --output=$@ $^

# Compile C++ .hh files
$(cxx_hh_syn): %.hh.syn: $(topdir)/%.hh
	@echo parsing $^
	if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) cxx --output=$@ $?

# Compile C++ .h files
$(cxx_h_syn): %.h.syn: $(topdir)/%.h
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) cxx --output=$@ $?

# Compile C++ .cc files
$(cxx_cc_syn): %.cc.syn: $(topdir)/%.cc
	@echo parsing $^
	@if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi
	$(synopsis) cxx --output=$@ $?

# Link Python files together
py.syn:	$(py_syn)
	@echo linking Python files together
	$(synopsis) link_python --output=$@ $^

# Link C++ files together
c++.syn: $(cxx_hh_syn) $(cxx_h_syn) $(cxx_cc_syn)
	@echo linking C++ files together
	@$(synopsis) link_cxx --output=$@ $^

# Link all files together
all.syn: py.syn c++.syn
	@echo linking all files together
	@$(synopsis) link --output=$@ $^

texi-help	:= aux cp cps fn fns ky log pg toc tp tps vr vrs

html-doxygen:   all.syn
	@echo formatting $@
	@$(synopsis) -Wc,formatter=Doxygen $^

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

