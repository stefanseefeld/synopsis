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

CCC = cc
CXX = c++
RANLIB =

OPTIMISE = -O6 -march=i686
#DEBUG = -ggdb -pg
DEBUG =

CFLAGS = $(OPTIMISE) $(DEBUG)
#CXXFLAGS = -O2 -I..
CXXFLAGS = $(OPTIMISE) $(DEBUG) -Wall -I..
GCLIB = ../gc/gc.a
LDFLAGS = $(DEBUG) # -export-dynamic	# for GNU ld
LIB =# -ldl			# Linux
PYLIB = -lpython1.5 -ldl -lpthread

OBJ = buffer.o hash.o token.o ptree.o ptree-core.o encoding.o env.o\
	pattern.o walker.o typeinfo.o parse.o mop.o classwalk.o\
	metaclass.o quote-class.o member.o cbodywalk.o

all : ../../occ.so

$(OBJ): %.o : ../%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

synopsis.o:	synopsis.cc synopsis.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

synopsis.g.o:	synopsis.cc synopsis.hh
	$(CXX) -ggdb -pg -DDEBUG $(CXXFLAGS) -c -o $@ $<

occ.o:	occ.cc synopsis.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

occ.g.o: occ.cc synopsis.hh
	$(CXX) -ggdb -pg -DDEBUG $(CXXFLAGS) -c -o $@ $<

../../occ.so : opencxx.a synopsis.o occ.o
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ occ.o synopsis.o opencxx.a $(LIB)

occ.gdb:	opencxx.a synopsis.g.o occ.g.o
	$(CXX) -ggdb -pg $(CXXFLAGS) $(LDFLAGS) -o $@ occ.g.o synopsis.g.o opencxx.a $(LIB) $(PYLIB)

opencxx.a : $(OBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OBJ)
#	$(RANLIB) $@

clean :
	rm -f *.[o] *.ii *.so *core opencxx.a ../../occ.so
