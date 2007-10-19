#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import ASG
import re

class MacroFilter(Processor, ASG.Visitor):
    """A MacroFilter allows macros to be filtered, based on pattern matching"""

    pattern = Parameter('', 'Regular expression to match macro names with.')

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        self._pattern = re.compile(self.pattern)
        self.ir = self.merge_input(ir)

        for decl in self.ir.declarations[:]:
            decl.accept(self)

        return self.output_and_return_ir()

    def visit_macro(self, node):

        if self._pattern.match(node.name[-1]):
            # Macros always live in the top-most scope.
            self.ir.declarations.remove(node)
