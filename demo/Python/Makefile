synopsis	:= ../../synopsis -p Python -f HTML -Wf,-s,style.css

all:

ast:	    AST.py
	    $(synopsis) -o AST $?

test:	    test.py
	    $(synopsis) -o test $?

clean:
		rm -f *~ *.html *.syn

distclean:	clean
