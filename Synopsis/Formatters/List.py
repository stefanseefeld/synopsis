#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST

class Formatter(Processor, AST.Visitor):
    """Generate a high-level list of the content of a syn file.
    This formatter can lists source files (by name), as well as
    declarations (by name, type) contained in a given scope."""

    show_files = Parameter(False, 'list files')
    show_scope = Parameter(None, 'list declarations in the given scope')

    scope_separator = {'C' : '::',
                       'C++' : '::',
                       'IDL' : '::',
                       'Python' : '.'}

    def process(self, ast, **kwds):

        self.set_parameters(kwds)
        self.ast = self.merge_input(ast)

        if self.show_files:
            for name, sf in self.ast.files().iteritems():
                print name, sf.name

        if self.show_scope is not None:
            if '.' in self.show_scope:
                self.scope = tuple(self.show_scope.split('.'))
            else:
                self.scope = tuple(self.show_scope.split('::'))
                
            # Now guess the current language by looking at the
            # first declaration.
            d = len(self.ast.declarations()) and self.ast.declarations()[0]
            sf = d and d.file() or None
            if not sf: # d was a MetaModule, so let's assume C++ or IDL.
                self.sep = '::'
            else:
                lang = sf.annotations.get('language', 'Python')
                self.sep = self.scope_separator[lang]


            for d in self.ast.declarations():
                d.accept(self)
                
        return self.ast


    def visitScope(self, node):

        if self.scope == node.name():

            # We found the right scope.
            # List all declarations directly contained in it.
            declarations = node.declarations()[:]
            declarations.sort(lambda x, y : cmp(x.name(), y.name()))
            for d in declarations:
                if isinstance(d, AST.Builtin): continue
                name = d.name()[-1]
                _type = d.type()
                print '%s : %s'%(name, _type)
        elif (len(node.name()) < self.scope and
              self.scope[0:len(node.name())] == node.name()):

            # We found a parent scope.
            # Visit child scopes.
            for d in node.declarations():
                d.accept(self)


__all__ = ['Formatter']
