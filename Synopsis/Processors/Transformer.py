#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Synopsis.Processor import Processor

class Transformer(Processor, AST.Visitor):
    """A class that creates a new AST from an old one. This is a helper base for
    more specialized classes that manipulate the AST based on
    the comments in the nodes"""

    def __init__(self, **kwds):
        """Constructor"""

        Processor.__init__(self, **kwds)
        self.__scopes = []
        self.__current = []

    def process(self, ast, **kwds):
      
        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        for decl in ast.declarations():
            decl.accept(self)

        self.finalize()
        return self.output_and_return_ast()

    def finalize(self):
        """replace the AST with the newly created one"""

        self.ast.declarations()[:] = self.__current

    def push(self):
        """Pushes the current scope onto the stack and starts a new one"""

        self.__scopes.append(self.__current)
        self.__current = []

    def pop(self, decl):
        """Pops the current scope from the stack, and appends the given
        declaration to it"""

        self.__current = self.__scopes.pop()
        self.__current.append(decl)

    def add(self, decl):
        """Adds the given decl to the current scope"""

        self.__current.append(decl)

    def current_scope(self):
        """Returns the current scope: a list of declarations"""

        return self.__current
