# $Id: Makefile,v 1.2 2003/08/20 03:25:16 stefan Exp $
#
# This source file is a part of the Synopsis Project
# Copyright (C) 2003 Stefan Seefeld
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
include ../../../local.mk
endif
endif

ifeq '$(python_prefix)' '/usr'
CPPFLAGS+= -D PYTHON_INCLUDE="$(python_include)"
else
CPPFLAGS+= -I $(python_prefix)/include -D PYTHON_INCLUDE="$(python_include)"
endif

CPPFLAGS+= $(CTOOL_CPPFLAGS)
LIBS	:= $(CTOOL_LIBS) $(LIBS)

SRC	:= Translator.cc ctool.cc
OBJ	:= $(patsubst %.cc, %.o, $(SRC))
DEP	:= $(patsubst %.cc, %.d, $(SRC))

MODULE	:= ctool.$(LIBRARY_EXT)

PY	:= __init__.py __init__.pyc $(MODULE)

all: $(MODULE)

$(MODULE): $(OBJ)
	$(CXX) -shared $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:

distclean: clean

install:
	mkdir -p $(packagedir)/Synopsis/Parser/C
	install -m 644 $(PY) $(packagedir)/Synopsis/Parser/C

%.o : %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.d : %.cc
	$(SHELL) -ec '$(MAKEDEP) $(CPPFLAGS) $< \
          | sed "s/$*\\.o[ :]*/$*\\.d $*\\.o : /g" > $@'

ifeq (,$(filter $(MAKECMDGOALS), depend doc clean distclean maintainer-clean))
-include $(DEP)
endif
