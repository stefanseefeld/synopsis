# $Id: Makefile,v 1.2 2001/04/18 05:44:52 chalky Exp $
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

SHELL	:= /bin/sh

ifeq (,$(findstring $(MAKECMDGOALS), clean distclean))
ifeq (,$(findstring $(action), clean distclean))
include ../local.mk
endif
endif

subdirs	:= IDL C++ Python Mixed Graph Config Perceps

action	:= all

.PHONY: all $(subdirs)

all:	$(subdirs)

$(subdirs):
	@echo making $(action) in $@
	$(MAKE) -C $@ $(action)

clean:
	$(MAKE) action="clean"
