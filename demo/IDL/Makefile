synopsis	:= ../../synopsis -p IDL -Wp,-K -f HTML -Wf,-s,style.css

all:

CosNaming:	CosNaming.idl
		$(synopsis) -o CosNaming $?

Types:		Types.idl
		$(synopsis) -o Types $?

Graphic:	Graphic.idl
		$(synopsis) -o Graphic $?

Input:		Input.idl
		$(synopsis) -o Input $?

clean:
		rm -f *~ *.html *.syn

distclean:	clean

install: