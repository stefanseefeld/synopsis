omniidl		:= omniidl -nf -bcxx
synopsis	:= ../../synopsis

#
# adapt to your environent...
#
CPPFLAGS	:= -I. -I/usr/local/include -Wp,-D__x86__

all:	interface implementation

Storage.hh:	Storage.idl
		$(omniidl) $?

Storage.syn:	Storage.idl
		$(synopsis) -p IDL -o $@ $<

StorableImpl.syn:	StorableImpl.hh Storage.hh
		$(synopsis) -p C++ -Wp,-m -Wl,-M,C++toIDL $(CPPFLAGS) -o $@ $<

FactoryImpl.syn:	FactoryImpl.hh Storage.hh
		$(synopsis) -p C++ -Wp,-m -Wl,-M,C++toIDL $(CPPFLAGS) -o $@ $<

interface interface.toc:	Storage.syn
		$(synopsis) -f HTML -Wf,-t,interface.toc -Wf,-s,style.css,-S,../html.css -o interface $<
		touch $@

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
		$(synopsis) -f HTML -Wf,-r,"interface.toc|../interface" -Wf,-s,style.css,-S,../html.css -o $@ StorableImpl.syn FactoryImpl.syn
		touch $@

clean:
	rm -rf Storage.hh StorageSK.cc *.syn interface interface.toc implementation
