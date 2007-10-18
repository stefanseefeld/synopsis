#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG

class ModuleSorter(Processor, ASG.Visitor):
    """A processor that sorts declarations in a module alphabetically."""

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        self.__scopestack = []
        self.__currscope = []

        for decl in self.ir.declarations:
            decl.accept(self)

        return self.output_and_return_ir()


    def visit_meta_module(self, module):
        """Visits all children of the module, and if there are no declarations
        after that removes the module"""

        def compare(a, b): return cmp(a.name(), b.name())
        module.declarations().sort(compare)
