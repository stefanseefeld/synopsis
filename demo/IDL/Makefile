synopsis = ../../synopsis -p IDL -Wp,-K -f HTML -Wf,-s,style.css

WARSAW_DIR=/mnt/src/berlin/Berlin/idl
warsaw_synopsis = ../../synopsis -p IDL -Wp,-K,-I,$(WARSAW_DIR) -f HTML -Wf,-s,style.css
WARSAW_ALL=$(wildcard $(WARSAW_DIR)/Warsaw/*.idl)

all.html:	$(WARSAW_ALL)
		echo "all = $(WARSAW_ALL)"
		$(warsaw_synopsis) -o all $(WARSAW_ALL)

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
