#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

SHELL	:= /bin/sh

subdirs	:= $(filter-out Paths/src, $(wildcard Paths/*))

action	:= all

.PHONY: all $(subdirs)

all:	$(subdirs)

$(subdirs):
	@echo making $(action) in $@
	$(MAKE) -k -C $@ $(action)

clean:
	$(MAKE) action="clean"

distclean:
	$(MAKE) action="distclean"

