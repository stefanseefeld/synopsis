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
texi-help	:= aux cp cps fn fns ky log pg toc tp tps vr vrs


TEXI		:= ast.texi type.texi util.texi \
            parsers-c++.texi parsers-idl.texi parsers-py.texi \
            processors.texi \
            formatters-ascii.texi formatters-html.texi formatters-dump.texi \
            formatters-dia.texi formatters-db.texi formatters-dot.texi \
            formatters-html-simple.texi formatters-texi.texi

all: html pdf info

pdf: Manual.pdf

vpath %.py $(topdir)
vpath %.cc $(topdir)
vpath %.h $(topdir)
vpath %.hh $(topdir)

html:   all.syn
	@echo formatting $@
	@$(synopsis) html --output=$@ $^

# Compile Python files
$(py_syn): %.syn: %.py
	@echo parsing $^
	mkdir -p $(dir $@)
	$(synopsis) python --output=$@ $^

# Compile C++ .hh files
$(cxx_hh_syn): %.hh.syn: $(topdir)/%.hh
	@echo parsing $^
	mkdir -p $(dir $@)
	$(synopsis) cxx --output=$@ $?

# Compile C++ .h files
$(cxx_h_syn): %.h.syn: $(topdir)/%.h
	@echo parsing $^
	mkdir -p $(dir $@)
	$(synopsis) cxx --output=$@ $?

# Compile C++ .cc files
$(cxx_cc_syn): %.cc.syn: $(topdir)/%.cc
	@echo parsing $^
	mkdir -p $(dir $@)
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

ast.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::AST' --output=$@ $^

type.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Type' --output=$@ $^

util.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Util' --output=$@ $^

parsers-c++-py.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Parsers::Cxx' --output=$@ $^

parsers-c++-cpp.syn:	c++.syn
	@echo linking $@
	$(synopsis) link --output=$@ $^

parsers-c++.syn:	parsers-c++-py.syn parsers-c++-cpp.syn
	@echo linking $@
	$(synopsis) link --output=$@ $^

parsers-idl.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Parsers::IDL' --output=$@ $^

parsers-py.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Parsers::Python' --output=$@ $^

processors.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Processors' --output=$@ $^

formatters-ascii.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::ASCII' --output=$@ $^

formatters-html.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::HTML' --output=$@ $^

formatters-dump.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::Dump' --output=$@ $^

formatters-dia.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::Dia' --output=$@ $^

formatters-db.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::DocBook' --output=$@ $^

formatters-dot.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::Dot' --output=$@ $^

formatters-html-simple.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::HTML_Simple' --output=$@ $^

formatters-texi.syn:	py.syn
	@echo linking $@
	$(synopsis) strip --scope='Synopsis::Formatters::TexInfo' --output=$@ $^

%.texi:   %.syn
	@echo formatting $@
	$(synopsis) texi --output=$@ $^

Manual.pdf:	$(TEXI) Manual.texi
	@echo generating $@
	texi2pdf Manual.texi

info:	$(TEXI) Manual.texi
	@echo generating info files
	makeinfo Manual.texi

install: all
	mkdir -p $(datadir)/Synopsis
	cp -pr html $(datadir)/Synopsis
	mkdir -p $(infodir)
	install -m644 synopsis.info* $(infodir)
	install -m644 Manual.pdf $(datadir)/Synopsis

clean:
	rm -rf Synopsis link xref *.syn links.toc $(TEXI) \
     $(addprefix Manual., $(texi-help))

distclean:  clean
	rm -rf html Manual.pdf *.info* 

