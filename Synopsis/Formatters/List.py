#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG

class Formatter(Processor, ASG.Visitor):
    """Generate a high-level list of the content of a syn file.
    This formatter can lists source files (by name), as well as
    declarations (by name, type) contained in a given scope."""

    show_files = Parameter(False, 'list files')
    show_scope = Parameter(None, 'list declarations in the given scope')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        if self.show_files:
            for f in self.ir.files.values():
                print '%s (language=%s, primary=%d)'%(f.name, f.annotations['language'],
                                                      f.annotations['primary'])

        if self.show_scope is not None:
            if '.' in self.show_scope:
                self.scope = tuple(self.show_scope.split('.'))
            elif '::' in self.show_scope:
                self.scope = tuple(self.show_scope.split('::'))
            else:
                self.scope = (self.show_scope,)
                
            for d in self.ir.asg.declarations:
                d.accept(self)
                
        return self.ir


    def visit_scope(self, node):

        if self.scope == node.name:

            # We found the right scope.
            # List all declarations directly contained in it.
            declarations = node.declarations[:]
            declarations.sort(lambda x, y : cmp(x.name, y.name))
            for d in declarations:
                if isinstance(d, ASG.Builtin): continue
                print '%s : %s'%(d.name[-1], d.type)
        elif (len(node.name) < self.scope and
              self.scope[0:len(node.name)] == node.name):

            # We found a parent scope.
            # Visit child scopes.
            for d in node.declarations:
                d.accept(self)
