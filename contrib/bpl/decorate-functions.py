#! /usr/bin/env python

import os, os.path, sys
sys.path.insert(0, os.getcwd())

import PTree
import SymbolLookup
from Processor import *

class FunctionDecorator(PTree.Visitor):
    """A FunctionDecorator walks over a parse tree and inserts comments at
    the beginning of function definitions."""

    def __init__(self, buffer):
        super(FunctionDecorator, self).__init__()
        self.__buffer = buffer

    def visit_list(self, l):
        if l.car():
            l.car().accept(self)
        if l.cdr():
            l.cdr().accept(self)

    def visit_declaration(self, d):
        third = d.third()
        if type(third) == PTree.Declarator and third.type.is_function:
            body = d.nth(3);
            # body.first() is '{', so we insert a comment after it
            comment = '\n// this is the start of %s'%third.name
            self.__buffer.insert(body.first(), comment)
            body.accept(self)


# create a buffer from the given input file
test = os.path.join(os.path.dirname(__file__), 'functions.cc')
buffer = Buffer(test)

# create a lexer operating on that buffer, using std C++ with gcc extensions
lexer = Lexer(buffer, TokenSet.CXX|TokenSet.GCC)

# create a dummy symbol lookup table (will use a real one once it is fully functional
symbols = SymbolLookup.Table(SymbolLookup.Language.NONE)

# create a parser with the above lexer and symbol table, using the std C++ grammar
parser = Parser(lexer, symbols, RuleSet.CXX)

# generate a parser tree
ptree = parser.parse()

# walk over the parse tree to insert comments at the beginning of functions
decorator = FunctionDecorator(buffer)
ptree.accept(decorator)

# write the modified buffer to the new file
buffer.write('functions-dec.cc', '')
