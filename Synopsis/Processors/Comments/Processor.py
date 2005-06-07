#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import Synopsis.Processor
from Synopsis import AST

class Processor(Synopsis.Processor.Processor, AST.Visitor):
    """Base class for comment processors.

    This is an AST visitor, and by default all declarations call process()
    with the current declaration. Subclasses may override just the process
    method.
    """
    def process(self, ast, **kwds):
      
        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        for decl in ast.declarations():
            decl.accept(self)

        self.finalize()
        return self.output_and_return_ast()

    def process_comments(self, decl):
        """Process comments for the given declaration."""
        pass

    def finalize(self):
        """Do any finalization steps that may be necessary."""
        pass

    def visitDeclaration(self, decl):
        self.process_comments(decl)

    def visitBuiltin(self, decl):
        if decl.type() == 'EOS': # treat it if it is an 'end of scope' marker
            self.process_comments(decl)

