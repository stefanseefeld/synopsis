synopsis:= python synopsis.py

src	:= Path.h Polyline.h Bezier.h Nurbs.h
syn	:= $(patsubst %.h, %.syn, $(src))

all: Paths

Paths: Paths.syn Paths.xref
	$(synopsis) html --output=$@ $<

Paths.syn: $(syn)
	$(synopsis) link --output=$@ $^

Paths.xref: Paths.syn 
	@echo generating $@
	$(synopsis) xref --output=$@ $^

Path.syn: ../src/Path.h
	$(synopsis) cxx_ssd_prev --output=$@ $<

Polyline.syn: ../src/Polyline.h
	$(synopsis) cxx_ss --output=$@ $<

Bezier.syn: ../src/Bezier.h
	$(synopsis) cxx_ssd --output=$@ $<

Nurbs.syn: ../src/Nurbs.h
	$(synopsis) cxx_javadoc --output=$@ $<

clean:
	rm -rf *.syn Paths.xref links xref

distclean: clean
	rm -rf Paths