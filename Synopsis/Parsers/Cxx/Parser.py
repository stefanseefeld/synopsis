# $Id: Parser.py,v 1.1 2003/11/05 17:36:55 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Parser for C++ using OpenC++ for low-level parsing.
This parser is written entirely in C++, and compiled into shared libraries for
use by python.
@see C++/Synopsis
@see C++/SWalker
"""

from Synopsis.Core.Processor import Processor
from Synopsis.Core import AST
import occ

class Parser(Processor):

   cppflags = []
   main_file_only = True
   preprocessor = 'gcc'
   base_path = ''
   extract_tails = True
   syntax_prefix = None
   xref_prefix = None
  
   def process(self, ast, **kwds):
        
      self.__dict__.update(kwds)

      for file in self.input:
         ast = occ.parse(ast, file, [],
                         self.verbose,
                         self.main_file_only,
                         self.base_path,
                         self.preprocessor,
                         self.cppflags,
                         self.extract_tails,
                         self.syntax_prefix,
                         self.xref_prefix)
      output = kwds.get('output')
      if output:
         AST.save(output, ast)
      return ast
