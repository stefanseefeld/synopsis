###############################################################################
#
#	ctree Makefile
#

CC     = gcc
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
CT_TARG = ctree
TG_TARG = tg
CPP_PATH =/lib/cpp

# For DOS boxes, you want these.
#RM = del
#MV = rename
#FLEXDFLT = lexyy.c
#CT_TARG = ctree.exe
#TG_TARG = tg.exe
#CPP_PATH =/usr/local/bin/cpp.exe

# Do _NOT_ remove the USE_CPP def, the other code is buggy!
CTREEFLAGS = \
    -DUSE_CPP \
    -DSHOW_TYPES \
    -DDEBUG

# For easy cut and paste in CTREEFLAGS
POSSIBLE_FLAGS = \
    -DUSE_CPP \
    -DSHOW_TYPES \
    -DDEBUG

LIBS = 
LIBSLOC=

###############################################################################
#
#	Source files and directories
#

DEMO_C_MAIN = ctree.c
DEMO_C_OBJ = ctree.o

LIBCT_FILES = \
    heap.c \
    nmetab.c \
    symtab.c \
    token.c \
    tree.c \
    tree_stk.c

LIBCT_OBJS = \
    heap.o \
    nmetab.o \
    symtab.o \
    token.o \
    tree.o \
    tree_stk.o

OTHER_FILES = lexer.o gram.o 

DEMO_C_FILES = prnttree.c
DEMO_C_OBJS = prnttree.o

LEX_FILES = lexer.l

YACC_FILES = gram.y

###############################################################################
#
#	Object files
#

OBJFILES = \
	$(LIBCT_OBJS) $(OTHER_FILES)

ALL_OBJS = \
	$(DEMO_C_OBJ) $(DEMO_C_OBJS) $(OBJFILES) 

###############################################################################
#
#	Other variables
#

INCLUDES= -I.
CPPFLAGS = -DLIB_CPP=\"$(CPP_PATH)\"

GCCFLAGS = -g -Wall

DEFINES=
CFLAGS=	$(GCCFLAGS) $(INCLUDES) $(DEBUGFLAGS) $(CTREEFLAGS) $(CPPFLAGS)
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

.SUFFIXES:	.C .d

.C.o:
	$(CPLUS) -c $(CPLUSFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $<

###############################################################################

.PHONY: all fresh tidy clean depend setup

all: tg

$(CT_TARG): gram.h $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $(CT_TARG) $(ALL_OBJS) $(LIBSLOC) $(LIBS)

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
	$(MAKE) ctree

tidy:
	$(RM) *.o
	$(RM) *.a
	$(RM) core
	$(RM) tg trigraph.c
	$(RM) lexer.c
	$(RM) gram.c
	$(RM) gram.h

clean: tidy
	$(RM) $(CT_TARG)
	$(RM) libct.a


###############################################################################
#
#	Special compilation rules
#

lexer.c: lexer.l gram.h
	$(FLEX) $(FLEXFLAGS) $<
	$(RM) lexer.c
	$(MV) $(FLEXDFLT) lexer.c

lexer.o: lexer.c
	$(CC) -c $(CFLAGS) $<

gram.o: gram.c
	$(CC) -c $(CFLAGS) $<

gram.c gram.h:	gram.y
	$(BISON) $(BISON_V) -d -b gram -o gram.c $<


###############################################################################
#
#	Dependencies
#

depend:	$(C_FILES) $(CPLUS_FILES) $(LEX_FILES) $(YACC_FILES)
	makedepend $(DEFINES) $(INCLUDES) $^

###############################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.
