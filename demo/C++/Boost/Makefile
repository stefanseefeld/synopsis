synopsis	:= ../../bin/synopsis -P boost.synopsis

all:
		@echo "The boost demo is not run by default. If you have the boost sources,"
		@echo "you must create a symlink to the boost root directory as 'boost' here"
		@echo "eg: ln -s ~/boost"
		@echo "Then you can run 'make boostdemo'"

boostdemo:	
		$(synopsis)


clean:
		rm -rf *~ *.html *.ps *.syn core *.aux *.fn *.cp *.tp *.vr *.log *.ky *.pg \
		       *.fns *.tps *.vrs *.pdf BoostPython BoostFiles BoostLinks BoostXRef

distclean:	clean
