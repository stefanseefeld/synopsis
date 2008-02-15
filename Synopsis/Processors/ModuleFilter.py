#
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG

class ModuleFilter(Processor, ASG.Visitor):
    """A processor that filters modules."""

    modules = Parameter([], 'List of modules to be filtered out.')
    remove_empty = Parameter(True, 'Whether or not to remove empty modules.')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        self.__scopestack = []
        self.__currscope = []

        for decl in self.ir.declarations:
           decl.accept(self)
        self.ir.declarations[:] = self.__currscope

        return self.output_and_return_ir()

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

    def visit_declaration(self, decl):
        """Adds declaration to scope"""

        self.add(decl)

    visit_builtin = visit_declaration
    visit_group = visit_declaration
    visit_scope = visit_declaration
    visit_enum = visit_declaration

    def visit_module(self, module):
        """Visits all children of the module, and if there are no declarations
        after that removes the module"""

        if module.name in self.modules:
            return
        
        self.push()
        for d in module.declarations:
            d.accept(self)
        module.declarations = self.__currscope
        # count the number of non-forward declarations in the current scope
        count = reduce(lambda x, y: x + y,
                       [not isinstance(x, (ASG.Forward, ASG.Builtin))
                                       for x in self.__currscope],
                       0)
        if not self.remove_empty or count: self.pop(module)
        else: self.pop_only()
