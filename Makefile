# $Id: Makefile,v 1.3 2000/03/07 05:27:18 stefan Exp $
#
# This source file is a part of the Synopsis Project
# Copyright (C) 2000 Stefan Seefeld <stefan@berlin-consortium.org> 
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

subdirs	= Synopsis Parser Formatter
# doc

all:
	for dir in ${subdirs}; do \
	   (cd $$dir && $(MAKE)) \
	    || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"; \

clean:
	/bin/rm -f *~
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) clean) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

distclean:
	@for dir in ${subdirs}; do \
	  (cd $$dir && $(MAKE) distclean) \
	  || case "$(MFLAGS)" in *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
