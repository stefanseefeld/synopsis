#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
"""Markup formatters."""

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import AST, Type

class Struct:

    def __init__(self, summary = '', details = ''):
        self.summary = summary
        self.details = details
        

class Formatter(Parametrized):
    """Interface class that takes a 'doc' annotation and formats its
    text. Markup-specific subclasses should provide appropriate format methods."""

    def init(self, processor):
      
        self.processor = processor

    def format(self, decl, view):
        """Format the declaration's documentation.
        @param view the View to use for references and determining the correct
        relative filename.
        @param decl the declaration
        @returns Struct containing summary / details pair.
        """

        doc = decl.annotations.get('doc')
        text = doc and doc.text or ''
        return Struct('', text)

    def lookup_symbol(self, symbol, scope):
        """Given a symbol and a scope, returns an URL.
        Various methods are tried to resolve the symbol. First the
        parameters are taken off, then we try to split the ref using '.' or
        '::'. The params are added back, and then we try to match this scoped
        name against the current scope. If that fails, then we recursively try
        enclosing scopes.
        """

        # Remove params
        index = symbol.find('(')
        if index >= 0:
            params = symbol[index:]
            symbol = symbol[:index]
        else:
            params = ''
        # Split ref
        if '.' in symbol:
            symbol = symbol.split('.')
        else:
            symbol = symbol.split('::')
        # Add params back
        symbol = symbol[:-1] + [symbol[-1] + params]
        # Find in all scopes
        scope = list(scope)
        while 1:
            entry = self._lookup_symbol_in(symbol, scope)
            if entry:
                return entry.link
            if len(scope) == 0: break
            del scope[-1]
        # Not found
        return None


    def _lookup_symbol_in(self, symbol, scope):

        # Try scope + ref[0]
        entry = self.processor.toc.lookup(scope + symbol[:1])
        if entry:
            # Found.
            if len(symbol) > 1:
                # Find sub-refs
                entry = self._lookup_symbol_in(symbol[1:], scope + symbol[:1])
                if entry:
                    # Recursive sub-ref was okay!
                    return entry 
            else:
                # This was the last scope in ref. Done!
                return entry
        # Try a method name match:
        if len(symbol) == 1:
            entry = self._find_method_entry(symbol[0], scope)
            if entry: return entry
        # Not found at this scope
        return None


    def _find_method_entry(self, name, scope):
        """Tries to find a TOC entry for a method adjacent to decl. The
        enclosing scope is found using the types dictionary, and the
        realname()'s of all the functions compared to ref."""

        try:
            scope = self.processor.ast.types()[scope]
        except KeyError:
            #print "No parent scope:",decl.name()[:-1]
            return None
        if not scope: return None
        if not isinstance(scope, Type.Declared): return None
        scope = scope.declaration()
        if not isinstance(scope, AST.Scope): return None
        for decl in scope.declarations():
            if isinstance(decl, AST.Function):
                if decl.realname()[-1] == name:
                    return self.processor.toc.lookup(decl.name())
        # Failed
        return None
