###############################################################################
#
#	cTool Makefile
#

CC     = gcc
CPLUS  = g++
AR     = ar
RANLIB = ranlib
FLEX   = flex 
FLEXFLAGS =
BISON   = bison 
#BISON_V = -v
BISON_V =

# For unix boxes, use these.
RM = rm -f
MV = mv
FLEXDFLT = lex.yy.c
CT_TARG = cgraph
#CPP_PATH =/lib/cpp
CPP_PATH =/usr/ccs/lib/cpp

# For DOS boxes, you want these.
#RM = del
#MV = rename
#FLEXDFLT = lexyy.c
#CT_TARG = cgraph.exe
#CPP_PATH =/usr/local/bin/cpp.exe

LIBS = -lct
LIBSLOC= -L../../src

###############################################################################
#
#	Source files and directories
#

DEMO_C_MAIN = cgraph.cpp
DEMO_C_OBJ = cgraph.o

###############################################################################
#
#	Object files
#

ALL_OBJS = \
	$(DEMO_C_OBJ)

###############################################################################
#
#	Other variables
#

INCLUDES= -I. -I../../src
CPPFLAGS = 

GCCFLAGS = -g -Wall

DEFINES=
CFLAGS=	$(GCCFLAGS) $(INCLUDES) $(DEBUGFLAGS) $(CTOOLFLAGS) $(CPPFLAGS)
CPLUSFLAGS= $(CFLAGS) 


#PURE_OPTS= -windows=no

###############################################################################
#
# For AIX (without gcc)
#
# CC= xlc
#
# CFLAGS += -D_ALL_SOURCE

###############################################################################
#
#	General compilation rules
#

.SUFFIXES:	.C .d .cpp

.C.o:
	$(CPLUS) -c $(CPLUSFLAGS) $<

.cpp.o:
	$(CPLUS) -c $(CPLUSFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $<

###############################################################################

.PHONY: all fresh tidy clean depend setup

all: $(CT_TARG)

$(CT_TARG): $(ALL_OBJS)
	$(CPLUS) $(CFLAGS) -o $(CT_TARG) $(ALL_OBJS) $(LIBSLOC) $(LIBS)

tidy:
	$(RM) *.o
	$(RM) core

clean: tidy
	$(RM) $(CT_TARG)

###############################################################################
#
#	Dependencies
#

#depend:	$(C_FILES) $(CPLUS_FILES) $(LEX_FILES) $(YACC_FILES)
#	makedepend $(DEFINES) $(INCLUDES) $^

###############################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.
