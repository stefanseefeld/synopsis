synopsis	:= ../../bin/synopsis -P boost.synopsis

all: BoostPython/index.html

BoostPython/index.html:
		$(synopsis)

clean:
		rm -rf *~ *.html *.ps *.syn core *.aux *.fn *.cp *.tp *.vr *.log *.ky *.pg \
		       *.fns *.tps *.vrs *.pdf BoostPython BoostFiles BoostLinks

distclean:	clean
