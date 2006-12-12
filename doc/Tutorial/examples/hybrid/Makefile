prefix:=
synopsis := python synopsis.py

all:	interface implementation

interface interface.toc: Storage.syn
	$(synopsis) format_idl --output=interface $<
	touch $@

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
	$(synopsis) format_cxx --output=$@ StorableImpl.syn FactoryImpl.syn
	touch $@

Storage.syn: Storage.idl
	$(synopsis) idl --output=$@ $<

StorableImpl.syn:	StorableImpl.hh Storage.hh
	$(synopsis) cxx --output=$@ $<

FactoryImpl.syn:	FactoryImpl.hh Storage.hh
	$(synopsis) cxx --output=$@ $<

install-html: all
ifdef prefix
	mkdir -p $(prefix)
	cp -r interface implementation $(prefix)
endif

clean:
	rm -rf *.syn interface.toc

distclean: clean
	rm -rf interface implementation 
