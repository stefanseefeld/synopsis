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

# FIXME - the i686 will need to be $configured
OPTIMISE	:= -O6 -march=i686
DEBUG		:= -O6 -march=i686 -ggdb -pg -DDEBUG 
#DEBUG		:= -O6 -DDEBUG

CFLAGS		:= 
CPPFLAGS	:= -I.. -I$(PYTHON_PREFIX)/include -DPYTHON_MAJOR=$(PYTHON_MAJOR) -DPYTHON_MINOR=$(PYTHON_MINOR)
CXXFLAGS	:= -Wall
GCLIB		:= ../gc/gc.a
LDFLAGS		:= # -export-dynamic	# for GNU ld
LIB		:= $(PYTHON_LIB) -ldl -lpthread # Linux

OCC_OBJ		:= buffer.o hash.o token.o ptree.o ptree-core.o encoding.o env.o\
		   pattern.o walker.o typeinfo.o parse.o mop.o classwalk.o\
		   metaclass.o quote-class.o member.o cbodywalk.o

SYN_OBJ		:= synopsis.o occ.o
SYN_DEBUG_OBJ	:= synopsis.o occ.o swalker.o ast.o builder.o type.o dict.o \
		   dumper.o decoder.o

SYN_GOBJ	= $(patsubst %.o, %.go, $(SYN_DEBUG_OBJ))

all : ../../../occ.so

# This compiles the OCC files in the parent dir
$(OCC_OBJ): %.o : ../%.cc
	$(CXX)  -I.. $(OPTIMISE) $(CXXFLAGS) -c -o $@ $<

# This compiles the parser files with optimisation
$(SYN_OBJ): %.o : %.cc
	$(CXX) $(OPTIMISE) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# This compiles debug versions of the object files with .go suffix
$(SYN_GOBJ): %.go : %.cc
	$(CXX) $(DEBUG) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# Include deps if they exist. 
-include .deps.go
-include .deps.o

# This creates the final python module
../../../occ.so : $(SYN_OBJ) opencxx.a
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

# This creates a binary with debugging and profiling that you can run
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
