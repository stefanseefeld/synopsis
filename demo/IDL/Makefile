synopsis = ../../synopsis -p IDL -Wp,-K -f HTML -Wf,-s,style.css


CosNaming.html:	CosNaming.idl
		$(synopsis) -o CosNaming $?

Types.html:	Types.idl
		$(synopsis) -o Types $?

Graphic.html:	Graphic.idl
		$(synopsis) -o Graphic $?

Input.html:	Input.idl
		$(synopsis) -o Input $?

clean:
		rm -f *~ *.html *.syn

distclean:	clean
