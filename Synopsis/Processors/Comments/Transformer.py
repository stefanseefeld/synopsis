#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Processor import Processor

class Transformer(Processor):
    """A class that creates a new AST from an old one. This is a helper base for
    more specialized classes that manipulate the AST based on
    the comments in the nodes"""

    def __init__(self, **kwds):
        """Constructor"""

        Processor.__init__(self, **kwds)
        self.__scopestack = []
        self.__currscope = []

    def finalize(self):
        """replace the AST with the newly created one"""

        self.ast.declarations()[:] = self.__currscope

    def push(self):
        """Pushes the current scope onto the stack and starts a new one"""

        self.__scopestack.append(self.__currscope)
        self.__currscope = []

    def pop(self, decl):
        """Pops the current scope from the stack, and appends the given
        declaration to it"""

        self.__currscope = self.__scopestack.pop()
        self.__currscope.append(decl)

    def add(self, decl):
        """Adds the given decl to the current scope"""

        self.__currscope.append(decl)

    def currscope(self):
        """Returns the current scope: a list of declarations"""

        return self.__currscope
