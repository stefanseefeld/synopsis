# $Id: Makefile,v 1.6 2003/11/19 04:56:52 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

SHELL	:= /bin/sh

subdirs	:= IDL C++/Employee C++/Shapes C++/GapBuffer \
           Python Perceps Mixed Graph

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

