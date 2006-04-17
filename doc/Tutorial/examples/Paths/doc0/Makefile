synopsis:= synopsis

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: Paths Paths.ps

Path: ../src/Path.h
	$(synopsis) -p Cxx -f HTML  -o $@ $<

Polyline: ../src/Polyline.h
	$(synopsis) -p Cxx -f HTML  -o $@ $<

Paths: Paths.syn
	$(synopsis) -f HTML --javadoc -o $@ $<

Paths.syn: $(syn)
	$(synopsis) -l Grouper1 -o $@ $^

Path.syn: ../src/Path.h
	$(synopsis) -p Cxx -l SSDFilter,Previous -o $@ $<

Polyline.syn: ../src/Polyline.h
	$(synopsis) -p Cxx -l SSFilter -o $@ $<

Bezier.syn: ../src/Bezier.h
	$(synopsis) -p Cxx -l SSDFilter -o $@ $<

Nurbs.syn: ../src/Nurbs.h
	$(synopsis) -p Cxx -l JavaFilter,JavaTags -o $@ $<

Paths.ps: Paths.syn
	$(synopsis) -f Dot -Wf,--title="class diagram" -Wf,--format=ps \
          -Wf,hide_operations=False,hide_attributes=False \
          -o $@ $<

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths Paths.ps
