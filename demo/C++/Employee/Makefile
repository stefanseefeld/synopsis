synopsis := python synopsis.py

all:	html

html: Employee-linked.syn
	$(synopsis) format --output=$@ $?

Employee-linked.syn: Employee.syn
	$(synopsis) link --output=$@ $?

Employee.syn: Employee.cc
	$(synopsis) parse --output=$@ $?

clean:
	rm -rf Employee.syn Employee-linked.syn *~ 

distclean: clean
	rm -rf html