synopsis = ../../synopsis -p IDL -Wp,-K -f HTML -Wf,-s,style.css

CosNaming.html:	CosNaming.idl
		$(synopsis) -o $@ $?

Types.html:	Types.idl
		$(synopsis) -o $@ $?

Graphic.html:	Graphic.idl
		$(synopsis) -o $@ $?

Input.html:	Input.idl
		$(synopsis) -o $@ $?

clean:
		rm -f *~ *.html *.syn

distclean:	clean