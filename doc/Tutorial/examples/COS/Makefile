prefix:=
synopsis := python synopsis.py

src:= CosNaming.idl CosTrading.idl CosTradingDynamic.idl CosTradingRepos.idl
syn:= $(patsubst %.idl, %.syn, $(src))

all: html

html: cos.syn
	$(synopsis) format --output=$@ $?
	touch $@

cos.syn: $(syn)
	$(synopsis) link --output=$@ $?

%.syn: %.idl
	$(synopsis) parse --output=$@ $?

install-html: all
ifdef prefix
	mkdir -p $(prefix)
	cp -r html/* $(prefix)
endif

install-print:

clean:
	rm -fr *.syn *~

distclean:	clean
	rm -rf html
