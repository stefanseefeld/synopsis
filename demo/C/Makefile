#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

SHELL	:= /bin/sh

CPPFLAGS:= pkg-config --cflags Synopsis
LIBS	:= pkg-config --libs Synopsis

TARGET	:= call-graph

call-graph: call-graph.o
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

%.o: %.cc
	@echo compiling $(@D)/$(@F)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean :
	rm -f call-graph
