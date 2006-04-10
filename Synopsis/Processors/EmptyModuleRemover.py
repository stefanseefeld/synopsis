#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST

class EmptyModuleRemover(Processor, AST.Visitor):
    """A processor that removes empty modules."""

    def process(self, ast, **kwds):
      
        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        self.__scopestack = []
        self.__currscope = []

        for decl in self.ast.declarations():
            decl.accept(self)
        self.ast.declarations()[:] = self.__currscope

        return self.output_and_return_ast()

    def push(self):
        """Pushes the current scope onto the stack and starts a new one"""

        self.__scopestack.append(self.__currscope)
        self.__currscope = []

    def pop(self, decl):
        """Pops the current scope from the stack, and appends the given
        declaration to it"""

        self.__currscope = self.__scopestack.pop()
        self.__currscope.append(decl)

    def pop_only(self):
        """Only pops, doesn't append to scope"""

        self.__currscope = self.__scopestack.pop()

    def add(self, decl):
        """Adds the given decl to the current scope"""

        self.__currscope.append(decl)

    def visitDeclaration(self, decl):
        """Adds declaration to scope"""

        self.add(decl)

    visitGroup = visitDeclaration
    visitEnum = visitDeclaration

    def visitModule(self, module):
        """Visits all children of the module, and if there are no declarations
        after that removes the module"""

        self.push()
        for decl in module.declarations():
            decl.accept(self)
        module.declarations()[:] = self.__currscope
        # count the number of non-forward declarations in the current scope
        count = len([d for d in self.__currscope
                     if not isinstance(d, AST.Forward)])

        if count:
            self.pop(module)
        else:
            self.pop_only()
