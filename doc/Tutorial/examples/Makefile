#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

SHELL	:= /bin/sh

prefix	:=

subdirs	:= $(filter-out Paths/src, $(wildcard Paths/*))
subdirs += COS
subdirs += hybrid
subdirs += math

action	:= all

.PHONY: all $(subdirs)

all:	$(subdirs)

$(subdirs):
	@echo making $(action) in $@
	$(MAKE) -k -C $@ $(action) prefix=$(prefix)/$@

install-html:
ifdef prefix
	mkdir -p $(prefix)
	cp index.html $(prefix)
endif
	$(MAKE) action="install-html" prefix=$(prefix)

install-print:
ifdef prefix
	mkdir -p $(prefix)
endif
	$(MAKE) action="install-print" prefix=$(prefix)

clean:
	$(MAKE) action="clean"

distclean:
	$(MAKE) action="distclean"

install-src:
ifdef prefix
	mkdir -p $(prefix)
	for file in $(shell find . -path '*/.svn' -prune -o \
          -name '*.syn' -prune -o \
          -name '*.xref' -prune -o \
          -name 'xref_db' -prune -o \
          -name '*.html' -prune -o \
          -name '*.toc' -prune -o \
          -name '*.css' -prune -o \
          -name '*.png' -prune -o \
          -name '*.ps' -prune -o \
          -type f -a -print); do \
          mkdir -p $(prefix)/`dirname $$file`; \
          cp -p $$file $(prefix)/$$file; \
        done
endif
