synopsis = ../../synopsis -p IDL -Wp,-K -f HTML -Wf,-s,style.css
synopsis2 = ../../synopsis -p IDL -Wp,-K -f HTML2 -Wf,-s,style.css

CN.html:	CosNaming.idl
		$(synopsis2) -o CN $?

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
