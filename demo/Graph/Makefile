synopsis	:= ../../synopsis -p C++ -f Dot

all: class# collaboration

class:	class.ps class.png class.map class.html
collaboration: col.ps

class.dot: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,dot -o $@ $<
class.ps: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,ps -o $@ $<
class.png: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,png -o $@ $<
class.map: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,map -o $@ $<
demo: demo.hh
	../../synopsis -p C++ -f HTML -Wf,-t,demo.toc -o $@ $<
	touch demo
class.html: demo.hh demo
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,html,-r,"demo.toc|demo" -o $@ $<
col.ps: demo.hh
	$(synopsis) -Wf,-c -Wf,-t,"collaboration graph" -o $@ $<

clean:
		rm -fr *~ *.ps *.png *.map *.html *.dot *.toc *.syn class

distclean:	clean
