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
CT_TARG = ctdemo
TG_TARG = tg
CPP_PATH =/lib/cpp

# For DOS boxes, you want these.
#RM = del
#MV = rename
#FLEXDFLT = lexyy.c
#CT_TARG = ctdemo.exe
#TG_TARG = tg.exe
#CPP_PATH =/usr/local/bin/cpp.exe

# MacOs X needs these changes
CC     = cc
CPLUS  = c++
CPP_PATH =/usr/bin/cpp

CTOOLFLAGS = \
    -DUSE_GCC_4_CPP \
    -DDEBUG

# For easy cut and paste in CTOOLFLAGS
POSSIBLE_FLAGS = \
    -DUSE_CPP_4_CPP \
    -DUSE_CAT_4_CPP \
    -DUSE_GCC_4_CPP \
    -DDEBUG

LIBS = 
LIBSLOC=

###############################################################################
#
#	Source files and directories
#

DEMO_C_MAIN = ctdemo.cpp
DEMO_C_OBJ = ctdemo.o

LIBCT_FILES = \
    context.cpp \
    decl.cpp \
    express.cpp \
    parseenv.cpp \
    project.cpp \
    stemnt.cpp \
    symbol.cpp \
    token.cpp \
    location.cpp	

LIBCT_OBJS = \
    context.o \
    decl.o \
    express.o \
    parseenv.o \
    project.o \
    stemnt.o \
    symbol.o \
    token.o \
    location.o

OTHER_FILES = lexer.o gram.o 

LEX_FILES = lexer.l

YACC_FILES = gram.y

###############################################################################
#
#	Object files
#

OBJFILES = \
	$(LIBCT_OBJS) $(OTHER_FILES) 

ALL_OBJS = \
	$(DEMO_C_OBJ) $(OBJFILES) 

###############################################################################
#
#	Other variables
#

INCLUDES= -I.
CPPFLAGS = -DLIB_CPP=\"$(CPP_PATH)\"

GCCFLAGS = -g -Wall
#GCCFLAGS = -O -Wall

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

.PHONY: all fresh tidy noexes clean release depend setup

all: $(CT_TARG) libct.a

$(CT_TARG): gram.h $(ALL_OBJS)
	$(CPLUS) $(CFLAGS) -o $(CT_TARG) $(ALL_OBJS) $(LIBSLOC) $(LIBS)

libct.a: $(OBJFILES)
	$(RM) $@
	$(AR) cr $@ $(OBJFILES) 
	$(RANLIB) $@

trigraph.c: trigraph.l
	$(FLEX) trigraph.l
	$(RM) trigraph.c
	$(MV) $(FLEXDFLT) trigraph.c

$(TG_TARG): trigraph.c
	$(CC) -O -o $(TG_TARG) trigraph.c -ll

fresh:
	$(MAKE) clean
	$(MAKE) $(CT_TARG)

tidy:
	$(RM) *.o
	$(RM) core

noexes: tidy
	$(RM) $(CT_TARG)
	$(RM) libct.a

clean: noexes
	$(RM) lexer.cpp
	$(RM) gram.cpp
	$(RM) gram.h

release: noexes
	cd ../regression; $(MAKE) pack

###############################################################################
#
#	Special compilation rules
#

lexer.cpp: lexer.l gram.h
	$(FLEX) $(FLEXFLAGS) $<
	$(RM) lexer.cpp
	$(MV) $(FLEXDFLT) lexer.cpp

lexer.o: lexer.cpp
	$(CPLUS) -c $(CFLAGS) $<

gram.o: gram.cpp
	$(CPLUS) -c $(CFLAGS) $<

gram.cpp gram.h:	gram.y
	$(BISON) $(BISON_V) -d -b gram -o gram.cpp $<
	$(MV) gram.cpp.h gram.h


###############################################################################
#
#	Dependencies
#

depend:	$(C_FILES) $(CPLUS_FILES) $(LEX_FILES) $(YACC_FILES)
	makedepend $(DEFINES) $(INCLUDES) $^

###############################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.

