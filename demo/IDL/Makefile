synopsis	:= ../../synopsis -p IDL -I. -Wp,-K -f HTML -Wf,--spec=html.spec

all: CosNaming Types Graphic Input

CosNaming:	CosNaming.idl
		$(synopsis) -o CosNaming $?

Types:		Types.idl
		$(synopsis) -o Types $?

Graphic:	Graphic.idl
		$(synopsis) -o Graphic $?

Input:		Input.idl
		../../synopsis -p IDL -I. -Wp,-K -f DocBook -o Input.xml $?

clean:
		rm -fr *~ *.html *.syn CosNaming Types Graphic Input

distclean:	clean
