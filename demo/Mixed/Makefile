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
		$(synopsis) -p C++ -Wp,-m -Wl,-m,crossref.py $(CPPFLAGS) -o $@ $<

FactoryImpl.syn:	FactoryImpl.hh Storage.hh
		$(synopsis) -p C++ -Wp,-m -Wl,-m,crossref.py $(CPPFLAGS) -o $@ $<

interface interface.toc:	Storage.syn
		$(synopsis) -f HTML -Wf,-t,interface.toc -Wf,-s,style.css,-S,style.css -o interface $<

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
		$(synopsis) -f HTML -Wf,-r,interface.toc -Wf,-s,style.css,-S,style.css -o $@ StorableImpl.syn FactoryImpl.syn

clean:
	rm -rf Storage.hh StorageSK.cc *.syn interface interface.toc implementation
