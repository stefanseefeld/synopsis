synopsis := python synopsis.py

all:	html sxr texi pdf

html: GapBuffer.xref GapBuffer.syn
	$(synopsis) html --output=$@ GapBuffer.syn

sxr:	GapBuffer.syn GapBuffer.xref
	$(synopsis) sxr --output=$@ $<

texi:	GapBuffer.texi

pdf:	texi
	texi2pdf Manual.texi

GapBuffer.syn xref/GapBuffer.hh links/GapBuffer.hh: GapBuffer.hh
	$(synopsis) parse --output=$@ $?

GapBuffer.xref: GapBuffer.syn xref/GapBuffer.hh
	$(synopsis) xref --output=$@ $<

GapBuffer.texi:	GapBuffer.syn
	$(synopsis) texinfo --output=$@ $?


clean:
	rm -rf links xref GapBuffer.xref GapBuffer.syn *~ \
           Manual.aux Manual.cp Manual.fn Manual.fns Manual.ky Manual.log \
           Manual.pg Manual.tp Manual.tps Manual.vr Manual.vrs

distclean: clean
	rm -rf html sxr GapBuffer.texi Manual.pdf