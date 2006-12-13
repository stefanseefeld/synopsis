prefix:=
synopsis:= synopsis

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: # by default do nothing

Path: ../src/Path.h
	$(synopsis) -p Cxx -f HTML  -o $@ $<

Polyline: ../src/Polyline.h
	$(synopsis) -p Cxx -f HTML  -o $@ $<

Paths: Paths.syn
	$(synopsis) -f HTML --cfilter=java --translate=javadoc -o $@ $<

Paths.syn: $(syn)
	$(synopsis) -l Comments.Grouper -o $@ $^

Path.syn: ../src/Path.h
	$(synopsis) -p Cxx --cfilter=ssd -l Comments.Previous -o $@ $<

Polyline.syn: ../src/Polyline.h
	$(synopsis) -p Cxx --cfilter=ss -o $@ $<

Bezier.syn: ../src/Bezier.h
	$(synopsis) -p Cxx --cfilter=ssd -o $@ $<

Nurbs.syn: ../src/Nurbs.h
	$(synopsis) -p Cxx --cfilter=java --translate=javadoc -o $@ $<

Paths.ps: Paths.syn
	$(synopsis) -f Dot -Wf,--title="class diagram" -Wf,--format=ps \
          -Wf,hide_operations=False,hide_attributes=False \
          -o $@ $<

install-html: # nothing to install

clean:
	rm -rf *.syn

distclean: clean
	rm -rf Paths Paths.ps
