synopsis:= ../../../../bin/synopsis

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

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

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths