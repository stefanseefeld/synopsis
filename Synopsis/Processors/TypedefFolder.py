#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import ASG

class TypedefFolder(Processor, ASG.Visitor):
    """Fold (anonymous) types into enclosing typedefs."""

    anonymous_only = Parameter(True, "Whether or not folding should be restricted to unnamed types.")

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        self.scopes = []
        # Iterate over a copy so we can safely modify
        # the original in the process.
        decls = ir.asg.declarations[:]
        for d in decls:
            d.accept(self)
        return self.output_and_return_ir()

    def visit_scope(self, s):

        self.scopes.append(s)
        decls = s.declarations[:]
        for d in decls:
            d.accept(self)
        self.scopes.pop()

    def visit_typedef(self, t):

        if t.constr:
            if self.verbose: print 'replace', t.alias.name, 'by', t.name
            alias = self.ir.asg.types.pop(t.alias.name)
            alias.declaration.name = alias.name = t.name
            self.ir.asg.types[alias.name] = alias
            if len(self.scopes):
                decls = self.scopes[-1].declarations
            else:
                decls = self.ir.asg.declarations
            del decls[decls.index(t)]

            if type(alias.declaration) == ASG.Class:
                i = len(alias.declaration.name)
                for d in alias.declaration.declarations:
                    d.name = d.name[:i-1] + (alias.name[-1],) + d.name[i:]
