omniidl		:= omniidl -nf -bcxx
synopsis	:= ../../synopsis -v -c config.py

#
# adapt to your environent...
#
CPPFLAGS	:= -I. -I/usr/local/include -Wp,-D__x86__

all:	interface implementation

Storage.hh:	Storage.idl
		$(omniidl) $?

Storage.syn:	Storage.idl
		$(synopsis) -Wc,parser=IDL,linker=Linker -o $@ $<

StorableImpl.syn:	StorableImpl.hh Storage.hh
		$(synopsis) -Wc,parser=C++,linker=Mapping $(CPPFLAGS) -o $@ $<

FactoryImpl.syn:	FactoryImpl.hh Storage.hh
		$(synopsis) -Wc,parser=C++,linker=Mapping $(CPPFLAGS) -o $@ $<

interface interface.toc:	Storage.syn
		$(synopsis) -Wc,formatter=Interface -o interface $<
		touch $@

implementation: StorableImpl.syn FactoryImpl.syn interface.toc
		$(synopsis) -Wc,formatter=Implementation -o $@ StorableImpl.syn FactoryImpl.syn
		touch $@

clean:
	rm -rf Storage.hh StorageSK.cc *.syn interface interface.toc implementation *.pyc *.pyo
