# $Id: Makefile,v 1.3 2001/01/21 03:46:29 stefan Exp $
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

SHELL	:= /bin/sh

include ../local.mk

#SRC	:= __init__.py Type.py AST.py Util.py Visitor.py 

CXX		:= c++
CPPFLAGS	:= -I. -I$(PYTHON_PREFIX)/include -DPYTHON_MAJOR=$(PYTHON_MAJOR) -DPYTHON_MINOR=$(PYTHON_MINOR)
CXXFLAGS	:= -Wall
LDFLAGS		:= 
LIB		:= $(PYTHON_LIB) -ldl -lpthread # Linux

SRC		:= Python.cc
OBJ		:= $(patsubst %.cc, %.o, $(SRC))

TARGET		:= Python.so

all: $(TARGET)

$(OBJ):	$(SRC)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(TARGET):	$(OBJ)
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

clean:
	/bin/rm -f *~

distclean:	clean
	/bin/rm -f *.pyc

install:
	mkdir -p $(packagedir)/Synopsis/Core
	install $(SRC) $(packagedir)/Synopsis/Core
