#
#  Copyright (C) 1997,1998 Shigeru Chiba, University of Tsukuba.
#
#  Permission to use, copy, distribute and modify this software and   
#  its documentation for any purpose is hereby granted without fee,        
#  provided that the above copyright notice appear in all copies and that 
#  both that copyright notice and this permission notice appear in 
#  supporting documentation.
#
#  Shigeru Chiba makes no representations about the suitability of this 
#  software for any purpose.  It is provided "as is" without express or
#  implied warranty.
#

#  For Linux

SHELL		:= /bin/sh

include ../../../../../local.mk

CCC		:= cc
CXX		:= c++
RANLIB		:=

OPTIMISE	:= -O2 -march=i686
#DEBUG = -ggdb -pg
DEBUG		:=

CFLAGS		:= $(OPTIMISE) $(DEBUG)
#CXXFLAGS = -O2 -I..
CPPFLAGS	:= -I.. -I$(PYTHON_PREFIX)/include -DPYTHON_MAJOR=$(PYTHON_MAJOR) -DPYTHON_MINOR=$(PYTHON_MINOR)
CXXFLAGS	:= $(OPTIMISE) $(DEBUG) -Wall
GCLIB		:= ../gc/gc.a
LDFLAGS		:= $(DEBUG) # -export-dynamic	# for GNU ld
LIB		:= $(PYTHON_LIB) -ldl -lpthread # Linux

OCC_OBJ		:= buffer.o hash.o token.o ptree.o ptree-core.o encoding.o env.o\
		   pattern.o walker.o typeinfo.o parse.o mop.o classwalk.o\
		   metaclass.o quote-class.o member.o cbodywalk.o

SYN_OBJ		:= synopsis.o occ.o
SYN_DEBUG_OBJ	:= synopsis.o occ.o swalker.o ast.o builder.o type.o dict.o \
		   dumper.o decoder.o

SYN_GOBJ	= $(patsubst %.o, %.go, $(SYN_DEBUG_OBJ))

all : ../../../occ.so

$(OCC_OBJ): %.o : ../%.cc
	$(CXX)  -I.. $(CXXFLAGS) -c -o $@ $<

$(SYN_OBJ): %.o : %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Note dep on .o to hackily inherit deps
$(SYN_GOBJ): %.go : %.cc
	$(CXX) -ggdb -pg -DDEBUG $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

-include .deps.go
-include .deps.o

synopsis.g.o:	synopsis.cc synopsis.hh
	$(CXX) -ggdb -pg -DDEBUG $(CXXFLAGS) -c -o $@ $<

occ.g.o: occ.cc synopsis.hh
	$(CXX) -ggdb -pg -DDEBUG $(CXXFLAGS) -c -o $@ $<

../../../occ.so : $(SYN_OBJ) opencxx.a
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

occ.gdb:	$(SYN_GOBJ) opencxx.a 
	$(CXX) -ggdb -pg $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB) $(PYLIB)

opencxx.a : $(OCC_OBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OCC_OBJ)
#	$(RANLIB) $@

depend:
	touch .deps.go
	touch .deps.o
	makedepend -DDEBUG -f.deps.go -o.go $(patsubst %.go,%.cc,$(SYN_GOBJ))
	makedepend -f.deps.o $(patsubst %.o,%.cc,$(SYN_OBJ))

clean :
	rm -f *.[o] *.ii *.so *core opencxx.a ../../../occ.so
