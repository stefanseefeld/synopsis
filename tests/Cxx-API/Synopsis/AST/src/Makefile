# $Id: Makefile,v 1.1 2004/01/11 19:45:21 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

SHELL	:= /bin/sh

python	:= python
PYPREFIX:=$(shell $(python) -c "import sys; print sys.prefix")
PYVERSION:=$(shell $(python) -c "from distutils import sysconfig; print sysconfig.get_config_var('VERSION')")
PYINC	:=$(shell $(python) -c "from distutils import sysconfig; print sysconfig.get_python_inc()")
PYLIBS	:=-L $(PYPREFIX)/lib/python$(PYVERSION)/config -lpython$(PYVERSION) -ldl -lutil -lpthread

CPPFLAGS:= -I ../../../../include -I $(PYINC)
LDFLAGS	:= -rdynamic
LIBS	:= $(PYLIBS)

TESTS	:= SourceFile

all:	$(TESTS)

%.o: %.cc
	$(CXX) $(CPPFLAGS) -ggdb $(CXXFLAGS) -c -o $@ $<

$(TESTS): %: %.o
	$(CXX) -ggdb $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o $(TESTS)
