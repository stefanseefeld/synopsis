synopsis	:= ../../synopsis -p C++ -f Dot

all: class# collaboration

class:	class.ps
collaboration: col.ps

class.ps: demo.hh
	$(synopsis) -Wf,-i -Wf,-t,"inheritance graph" -o class.ps demo.hh
col.ps: demo.hh
	$(synopsis) -Wf,-c -Wf,-t,"collaboration graph" -o col.ps demo.hh

clean:
		rm -f *~ *.ps *.syn

distclean:	clean
