synopsis := python synopsis.py

all: class# collaboration

class:	class.ps class.html
collaboration: collaboration.ps collaboration.html

class.dot: demo.syn
	$(synopsis) dot --type='inheritance' --format='dot' \
               --title='inheritance graph' --output=$@ $<
class.ps: demo.syn
	$(synopsis) dot --type='inheritance' \
               --title='inheritance graph' --output=$@ $<

class.html: demo.syn
	$(synopsis) dot --type='inheritance' --format='html' \
               --title='inheritance graph' --output=$@ $<

collaboration.ps: demo.syn
	$(synopsis) dot --type='collaboration' \
               --title='inheritance graph' --output=$@ $<

collaboration.html: demo.syn
	$(synopsis) dot --type='collaboration' --format='html' \
               --title='inheritance graph' --output=$@ $<

demo.syn: demo.hh
	$(synopsis) parse --output=$@ $?

clean:
	rm -rf demo.syn

distclean:	clean
		rm -fr *~ *.ps *.png *.map *.html *.dot *.toc *.syn demo
