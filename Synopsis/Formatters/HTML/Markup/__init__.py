#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
"""Markup formatters."""

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import ASG
from Synopsis.QualifiedName import *
from Synopsis.Formatters.HTML.Tags import escape
import re

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
        text = doc and escape(doc.text) or ''
        m = re.match(r'(\s*[\w\W]*?\.)(\s|$)', text)
        summary = m and m.group(1) or ''
        return Struct(summary, text)

    def lookup_symbol(self, symbol, scope):
        """Given a symbol and a scope, returns an URL.
        Various methods are tried to resolve the symbol. First the
        parameters are taken off, then we try to split the symbol using '.' or
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
        if '.' in symbol:
            symbol = QualifiedPythonName(symbol.split('.'))
        else:
            symbol = QualifiedCxxName(symbol.split('::'))
        # Add params back
        symbol = symbol[:-1] + (symbol[-1] + params,)
        # Find in all scopes
        while 1:
            entry = self._lookup_symbol_in(symbol, scope)
            if entry:
                return entry.link
            if len(scope) == 0: break
            scope = scope[:-1]
        # Not found
        return None


    def _lookup_symbol_in(self, symbol, scope):

        paren = symbol[-1].find('(')
        if paren:
            return self._find_method_entry(symbol[-1], scope + symbol[:-1])
        else:
            return self.processor.toc.lookup(scope + symbol)


    def _find_method_entry(self, name, scope):

        try:
            scope = self.processor.ir.types[scope]
        except KeyError:
            return None
        if not isinstance(scope, ASG.Declared):
            return None
        scope = scope.declaration
        if not isinstance(scope, ASG.Scope):
            return None
        # For now disregard parameters during lookup.
        name = name[:name.find('(')]
        for d in scope.declarations:
            if isinstance(d, ASG.Function):
                if d.real_name[-1] == name:
                    return self.processor.toc.lookup(d.name)
        return None
