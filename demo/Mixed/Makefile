synopsis	:= python synopsis.py
omniidl	:= omniidl

#
# adapt to your environent...
#
CPPFLAGS	:= -I. -I/usr/local/include -Wp,-D__x86__

all:	interface implementation

interface interface.toc: Storage.syn
	$(synopsis) format_idl --output=interface $<
	touch $@

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
	$(synopsis) format_cxx --output=$@ StorableImpl.syn FactoryImpl.syn
	touch $@

Storage.hh:	Storage.idl
	$(omniidl) -nf -bcxx $?

Storage.syn: Storage.idl
	$(synopsis) idl --output=$@ $<

StorableImpl.syn:	StorableImpl.hh Storage.hh
	$(synopsis) cxx --output=$@ $<

FactoryImpl.syn:	FactoryImpl.hh Storage.hh
	$(synopsis) cxx --output=$@ $<

clean:
	rm -rf Storage.hh StorageSK.cc *.syn interface.toc

distclean: clean
	rm -rf interface implementation 