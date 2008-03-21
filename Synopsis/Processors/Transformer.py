#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
from Synopsis.Processor import Processor

class Transformer(Processor, ASG.Visitor):
    """A class that creates a new ASG from an old one. This is a helper base for
    more specialized classes that manipulate the ASG based on
    the comments in the nodes"""

    def __init__(self, **kwds):
        """Constructor"""

        Processor.__init__(self, **kwds)
        self.__scopes = []
        self.__current = []

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        for decl in ir.asg.declarations:
            decl.accept(self)

        self.finalize()
        return self.output_and_return_ir()

    def finalize(self):
        """replace the ASG with the newly created one"""

        self.ir.asg.declarations[:] = self.__current

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

    def visit_builtin(self, decl):

        self.visit_declaration(decl)
