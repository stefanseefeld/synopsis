synopsis	:= ../../synopsis -Wc,parser=Python,formatter=HTML -Wf,-s,style.css,-S,../html.css

all: ast #test

ast:	    AST.py
	    $(synopsis) -o ast $?

#test:	    test.py
#	    $(synopsis) -o test $?

clean:
		rm -fr *~ *.html *.syn ast #test

distclean:	clean
