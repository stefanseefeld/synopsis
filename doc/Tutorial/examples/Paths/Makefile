synopsis:= ../../../../bin/synopsis

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: Paths Paths.ps

Paths: Paths.syn
	$(synopsis) -f HTML --javadoc -o $@ $<

Paths.syn: $(syn)
	$(synopsis) -l Grouper1 -o $@ $^

Path.syn: Path.h
	$(synopsis) -p Cxx -l SSDComments,Previous -o $@ $<

Polyline.syn: Polyline.h
	$(synopsis) -p Cxx -l SSComments -o $@ $<

Bezier.syn: Bezier.h
	$(synopsis) -p Cxx -l SSDComments -o $@ $<

Nurbs.syn: Nurbs.h
	$(synopsis) -p Cxx -l JavaComments,JavaTags -o $@ $<

Paths.ps: Paths.syn
	$(synopsis) -f Dot -Wf,--title="class diagram" -Wf,--format=ps \
          -Wf,hide_operations=False,hide_attributes=False \
          -o $@ $<

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths Paths.ps