synopsis	:= ../../synopsis -p C++ -f Dot

all: class# collaboration

class:	class.ps class.png class.map class.html
collaboration: col.ps

class.ps: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,"ps" -o class.ps demo.hh
class.png: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,"png" -o class.png demo.hh
class.map: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,"map" -o class.map demo.hh
class.html: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph",-f,"html" -o class.html demo.hh
col.ps: demo.hh
	$(synopsis) -Wf,-c -Wf,-t,"collaboration graph" -o col.ps demo.hh

clean:
		rm -f *~ *.ps *.png *.map *.html *.dot *.syn

distclean:	clean
