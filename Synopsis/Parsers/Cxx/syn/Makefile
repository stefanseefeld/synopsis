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

# Include local config stuff
ifeq (,$(findstring $(MAKECMDGOALS), clean distclean))
ifeq (,$(findstring $(action), clean distclean))
include ../../../../../local.mk
endif
endif

# Options (should be in local.mk?)
CCC		:= cc
CXX		:= c++
RANLIB		:=

# Options for optimised or debug versions
OPTIMISE	:= -O6 -fpic
DEBUG		:= -O1 -ggdb -pg -DDEBUG 

# Flags
CFLAGS		:= 
CPPFLAGS	:= -I.. -I$(PYTHON_PREFIX)/include -DPYTHON_MAJOR=$(PYTHON_MAJOR) -DPYTHON_MINOR=$(PYTHON_MINOR)
CXXFLAGS	:= -Wall
LDFLAGS		:=
LIB		:= $(PYTHON_LIB) -ldl -lpthread # Linux

GCLIB		:= ../gc/gc.a
OCC_SO		:= ../../../occ.so
LINK_SO		:= ../../../link.so


OCC_FILES	:= buffer hash token ptree ptree-core encoding env\
		   pattern walker typeinfo parse mop classwalk\
		   metaclass quote-class member cbodywalk

SYN_FILES	:= synopsis occ swalker ast builder type dict \
		   dumper decoder

# Generate sources, objs and debug-objs from the FILES
OCC_SOURCES	:= $(patsubst %,../%.cc,$(OCC_FILES))
OCC_OBJ		:= $(patsubst %,%.o,$(OCC_FILES))
OCC_GOBJ	:= $(patsubst %,%.go,$(OCC_FILES))

SYN_SOURCES	:= $(patsubst %,%.cc,$(SYN_FILES))
SYN_OBJ		:= $(patsubst %,%.o,$(SYN_FILES))
SYN_GOBJ	:= $(patsubst %,%.go,$(SYN_FILES))

TARGETS		= $(OCC_SO) $(LINK_SO)

all : $(TARGETS)

debug:		occ.gdb link-synopsis

# This compiles the OCC files in the parent dir
$(OCC_OBJ): %.o : ../%.cc
	$(CXX)  -I.. $(OPTIMISE) $(CXXFLAGS) -c -o $@ $<

# This compiles the parser files with optimisation
$(SYN_OBJ): %.o : %.cc
	$(CXX) $(OPTIMISE) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# This compiles DEBUG versions of the OCC files in the parent dir
$(OCC_GOBJ): %.go : ../%.cc
	$(CXX)  -I.. $(DEBUG) $(CXXFLAGS) -c -o $@ $<

# This compiles DEBUG versions of the object files with .go suffix
$(SYN_GOBJ): %.go : %.cc
	$(CXX) $(DEBUG) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

link-synopsis:	link.cc
	$(CXX) -DSTANDALONE $(DEBUG) $(CPPFLAGS) $(CXXFLAGS) -o link-synopsis link.cc


# Include deps if they exist. 
-include .deps.go
-include .deps.o

# This creates the final occ python module
$(OCC_SO): $(SYN_OBJ) opencxx.a
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

# This creates the final link python module
$(LINK_SO): link.cc
	$(CXX) -shared $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

# This creates a binary with debugging and profiling that you can run
occ.gdb:	$(SYN_GOBJ) opencxx.ga
	$(CXX) -ggdb -pg $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB) $(PYLIB)

# Compile an archive of the OCC files
opencxx.a : $(OCC_OBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OCC_OBJ)

# Compile a DEBUG archive of the OCC files
opencxx.ga : $(OCC_GOBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OCC_GOBJ)


depend:
	touch .deps.go
	touch .deps.o
	makedepend -Y -I.. -DDEBUG -f.deps.go -o.go $(SYN_SOURCES) $(OCC_SOURCES) 2> /dev/null
	makedepend -Y -I.. -f.deps.o $(SYN_SOURCES) $(OCC_SOURCES) 2> /dev/null

clean :
	rm -f *.[o] *.go *.ii *.so *core opencxx.a ../../../occ.so

