# $Id: Parser.py,v 1.1 2004/01/02 03:58:47 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Preprocessor for C, C++, IDL
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST

class Parser(Processor):

   preprocessor = Parameter(None, 'the preprocessor to use')
   emulate_compiler = Parameter('c++', 'a compiler to emulate')
   flags = Parameter([], 'list of preprocessor flags such as -I or -D')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = ast

      flags = self.flags
      if not self.preprocessor:
         import ucpp
         info = self.get_compiler_info(self.emulate_compiler)
         flags += map(lambda x:'-I%s'%x, info.include_paths)
         flags += map(lambda x:'-D%s=%s'%(x[0], x[1]), info.macros)
         for file in self.input:
            self.ast = ucpp.parse(self.ast, file, self.output, flags, self.verbose, self.debug)
      else:
         print 'not implemented yet: spawn external preprocessor'
      return self.output_and_return_ast()

   def get_compiler_info(self, compiler):
      import emul
      return emul.get_compiler_info(compiler)
