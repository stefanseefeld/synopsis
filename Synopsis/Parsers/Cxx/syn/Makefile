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

CCC = g++
CXX = g++
RANLIB =

CFLAGS = -O2
CXXFLAGS = -O2 -I..
GCLIB = ../gc/gc.a
LDFLAGS =# -export-dynamic	# for GNU ld
LIB =# -ldl			# Linux

OBJ = buffer.o hash.o token.o ptree.o ptree-core.o encoding.o env.o\
	pattern.o walker.o typeinfo.o parse.o mop.o classwalk.o\
	metaclass.o quote-class.o member.o cbodywalk.o

PYOBJ = occ.o

all : ../../occ.so

$(OBJ): %.o : ../%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(PYOBJ): %.o : %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

../../occ.so : opencxx.a $(PYOBJ)
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $(PYOBJ) opencxx.a $(LIB)

opencxx.a : $(OBJ)
	cp $(GCLIB) $@
	ar rcs $@ $(OBJ)
#	$(RANLIB) $@

clean :
	rm -f *.[o] *.ii *.so *core opencxx.a ../../occ.so
