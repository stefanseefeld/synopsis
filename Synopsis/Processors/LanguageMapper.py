# $Id: LanguageMapper.py,v 1.2 2003/11/11 02:57:57 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import string

from Synopsis.Processor import Processor, Parameter
from Synopsis.Core import AST, Type, Util

class LanguageMapper(Processor):

   def execute(self, ast):
      declarations = ast.declarations()
      langs = {}
      for decl in declarations:
         lang = decl.language()
         if langs.has_key(lang):
            scope = langs[lang]
         else:
            scope = AST.Module('',-1,lang,'Language',('%s Namespace'%lang,))
            langs[lang] = scope
         scope.declarations().append(decl)
      keys = langs.keys()
      # TODO: allow user to specify sort order here
      keys.sort()
      declarations[:] = map(lambda key, dict=langs: dict[key], keys)

linkerOperation = LanguageMapper
