synopsis	:= ../../synopsis -c config.py -I. -Wc,parser=IDL,linker=Linker,formatter=HTML

all: CosNaming Types Graphic Input

CosNaming:	CosNaming.idl
		$(synopsis) -o CosNaming $?

Types:		Types.idl
		$(synopsis) -o Types $?

Graphic:	Graphic.idl
		$(synopsis) -o Graphic $?

Input:		Input.idl
		../../synopsis -c config.py -Wc,parser=IDL,linker=Linker,formatter=DocBook -o Input.xml $?

clean:
		rm -fr *~ *.html *.syn CosNaming Types Graphic Input

distclean:	clean
