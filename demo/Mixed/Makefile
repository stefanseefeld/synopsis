omniidl		:= omniidl -nf -bcxx
synopsis	:= ../../synopsis

#CPPFLAGS	:= -I. -I/home/stefan/omni/include -Wp,-D__x86__
CPPFLAGS	:= -I. -I/home/chalky/src/omni/include -Wp,-D__x86__

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
		$(synopsis) -f HTML -Wf,-t,interface.toc -o interface $<

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
		$(synopsis) -f HTML -Wf,-r,interface.toc -o $@ StorableImpl.syn FactoryImpl.syn

clean:
	rm -rf Storage.hh StorageSK.cc *.syn interface interface.toc implementation
