# $Id: Makefile,v 1.17 2003/02/01 05:43:03 chalky Exp $
#
# This source file is a part of the Synopsis Project
# Copyright (C) 2000 Stefan Seefeld
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
# MA 02139, USA.

SHELL	= /bin/sh

ifeq (,$(findstring $(MAKECMDGOALS), clean distclean))
ifeq (,$(findstring $(action), clean distclean))
include ../../local.mk
endif
endif

SRC	:= __init__ TOC DUMP ASCII HTML_Simple Dot DocBook Dia ClassTree xref
PY	:= $(patsubst %, %.py, $(SRC)) $(patsubst %, %.pyc, $(SRC))

subdirs	:= HTML

action	:= all

.PHONY: all $(subdirs) clean distclean install

all:	$(subdirs)

$(subdirs):
	@echo making $(action) in $@
	$(MAKE) -C $@ $(action)

clean:
	$(MAKE) action="clean"
	/bin/rm -f *~ *.pyo *.pyc

distclean:
	$(MAKE) action="distclean"
	/bin/rm -f  *~ *.pyc

install:
	mkdir -p $(packagedir)/Synopsis/Formatter
	install -m 644 $(PY) $(packagedir)/Synopsis/Formatter
	$(MAKE) action="install"
