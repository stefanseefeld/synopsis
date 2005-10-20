#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import AST
import re

class MacroFilter(Processor, AST.Visitor):
    """A MacroFilter allows macros to be filtered, based on pattern matching"""

    pattern = Parameter('', 'Regular expression to match macro names with.')

    def process(self, ast, **kwds):
      
        self.set_parameters(kwds)
        self._pattern = re.compile(self.pattern)
        self.ast = self.merge_input(ast)

        for decl in ast.declarations():
            decl.accept(self)

        return self.output_and_return_ast()

    def visitMacro(self, node):

        if self._pattern.match(node.name()[-1]):
            # Macros always live in the top-most scope.
            self.ast.declarations().remove(node)
