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
include ../../../../../../local.mk
endif
endif

# Options for optimised or debug versions
OPTIMISE	:= -O6 -fPIC
DEBUG		:= -O1 -fPIC -ggdb -pg -DDEBUG -fno-inline

# Flags
CFLAGS		:= 
ifeq '$(python_prefix)' '/usr'
CPPFLAGS	:= -I.. -DPYTHON_INCLUDE="$(python_include)"
else
CPPFLAGS	:= -I.. -I$(python_prefix)/include -DPYTHON_INCLUDE="$(python_include)"
endif
CXXFLAGS	:= -Wall
LDFLAGS		:=
UCPP_FLAGS	:= -DSYNOPSIS -DSTAND_ALONE -DAUDIT -DMEM_CHECK
UCPP_AFLAGS	:= -DSTAND_ALONE -DAUDIT -DMEM_CHECK
LIB		:= $(python_lib) -ldl -lpthread

GCLIB		:= ../gc/gc.a
UCPP_LIB	:= ../ucpp/ucpp.a
UCPP_GLIB	:= ../ucpp/ucpp.ga
OCC_SO		:= ../../../occ.so
LINK_SO		:= ../../../link.so


OCC_FILES	:= buffer hash token ptree ptree-core encoding env\
		   pattern walker typeinfo parse mop classwalk\
		   metaclass quote-class member cbodywalk

SYN_FILES	:= synopsis occ swalker ast builder type dict \
		   dumper decoder swalker-syntax link_map linkstore

UCPP_FILES	:= mem hash cpp lexer assert macro eval

# Generate sources, objs and debug-objs from the FILES
OCC_SOURCES	:= $(patsubst %,../%.cc,$(OCC_FILES))
OCC_OBJ		:= $(patsubst %,%.o,$(OCC_FILES))
OCC_GOBJ	:= $(patsubst %,%.go,$(OCC_FILES))

SYN_SOURCES	:= $(patsubst %,%.cc,$(SYN_FILES))
SYN_OBJ		:= $(patsubst %,%.o,$(SYN_FILES))
SYN_GOBJ	:= $(patsubst %,%.go,$(SYN_FILES))

UCPP_SOURCES	:= $(patsubst %,../ucpp/%.c,$(UCPP_FILES))
UCPP_OBJ	:= $(patsubst %,../ucpp/%.o,$(UCPP_FILES))
UCPP_GOBJ	:= $(patsubst %,../ucpp/%.go,$(UCPP_FILES))
UCPP_AOBJ	:= $(patsubst %,../ucpp/%.ao,$(UCPP_FILES))

TARGETS		= $(OCC_SO) $(LINK_SO)

all : $(TARGETS)

debug:		occ.gdb link-synopsis ucpp

# This compiles the OCC files in the parent dir
$(OCC_OBJ): %.o : ../%.cc
	$(CXX)  $(OPTIMISE) -I.. $(CXXFLAGS) -c -o $@ $<

# This compiles the parser files with optimisation
$(SYN_OBJ): %.o : %.cc
	$(CXX) $(OPTIMISE) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# This compiles the UCPP files from ../ucpp
$(UCPP_OBJ): %.o : %.c
	$(CC) $(OPTIMISE) $(CFLAGS) $(UCPP_FLAGS) -c -o $@ $<

# This compiles DEBUG versions of the OCC files in the parent dir
$(OCC_GOBJ): %.go : ../%.cc
	$(CXX)  -I.. $(DEBUG) $(CXXFLAGS) -c -o $@ $<

# This compiles DEBUG versions of the object files with .go suffix
$(SYN_GOBJ): %.go : %.cc
	$(CXX) $(DEBUG) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

# This compiles DEBUG versions of the UCPP files
$(UCPP_GOBJ): %.go : %.c
	$(CC) $(DEBUG) $(CFLAGS) $(UCPP_FLAGS) -c -o $@ $<

# This compiles STANDALONE versions of the UCPP files
$(UCPP_AOBJ): %.ao : %.c
	$(CC) $(DEBUG) $(CFLAGS) $(UCPP_AFLAGS) -c -o $@ $<

link-synopsis:	link.cc
	$(CXX) -DSTANDALONE $(DEBUG) $(CPPFLAGS) $(CXXFLAGS) -o link-synopsis link.cc

ucpp:		$(UCPP_AOBJ)
	$(CC) -o $@ $^

# Include deps if they exist. 
-include .deps.ao
-include .deps.go
-include .deps.o

# This creates the final occ python module
$(OCC_SO): $(SYN_OBJ) opencxx.a $(UCPP_LIB)
	$(CXX) -shared $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

# This creates the final link python module
$(LINK_SO): link.cc
	$(CXX) -shared $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB)

# This creates a binary with debugging and profiling that you can run
occ.gdb:	$(SYN_GOBJ) opencxx.ga  $(UCPP_GLIB)
	$(CXX) -ggdb -pg $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIB) $(PYLIB)

# Compile an archive of the OCC files
opencxx.a : $(OCC_OBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OCC_OBJ)

# Compile a DEBUG archive of the OCC files
opencxx.ga : $(OCC_GOBJ) $(GCLIB)
	cp $(GCLIB) $@
	ar rcs $@ $(OCC_GOBJ)

# Compile an archive of UCPP files
$(UCPP_LIB):	$(UCPP_OBJ)
	ar rcs $@ $^

# Compile a DEBUG archive of UCPP files
$(UCPP_GLIB):	$(UCPP_GOBJ)
	ar rcs $@ $^

depend:
	touch .deps.ao
	touch .deps.go
	touch .deps.o
	makedepend -Y -I.. -DDEBUG -f.deps.ao -o.ao $(UCPP_SOURCES) 2> /dev/null
	makedepend -Y -I.. -DDEBUG -f.deps.go -o.go $(UCPP_SOURCES) $(SYN_SOURCES) $(OCC_SOURCES) 2> /dev/null
	makedepend -Y -I.. -f.deps.o $(SYN_SOURCES) $(UCPP_SOURCES) $(OCC_SOURCES) 2> /dev/null

clean :
	rm -f *~ *.o *.go *.ii *.so *core opencxx.a opencxx.ga occ.gdb \
	../../../occ.so ../../../link.so ../ucpp/*.o ../ucpp/*.go ../ucpp/*.a ../ucpp/*.ga

