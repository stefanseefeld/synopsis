synopsis = ../../bin/synopsis -p IDL -Wp,-K -f Dia

all:

CosNaming:	CosNaming.idl
		$(synopsis) -o cosnaming.dia $?

Types:		Types.idl
		$(synopsis) -o types.dia $?

Graphic:	Graphic.idl
		$(synopsis) -o graphic.dia $?

Input:		Input.idl
		$(synopsis) -o input.dia $?

test:		test.idl
		$(synopsis) -o test.dia $?

clean:
		rm -f *~ *.html *.syn *.pyc *.pyo

distclean:	clean
