synopsis := ../../bin/synopsis -v

all: html

html: API.syn
	rm -rf $@
	$(synopsis) -f HTML -o $@ $<

API.syn: API.h
	$(synopsis) -p C -o $@ $<