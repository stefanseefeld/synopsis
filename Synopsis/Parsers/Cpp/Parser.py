# $Id: Parser.py,v 1.3 2004/01/13 07:44:25 stefan Exp $
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

import os.path

class Parser(Processor):

   preprocessor = Parameter(None, 'the preprocessor to use')
   emulate_compiler = Parameter('c++', 'a compiler to emulate')
   flags = Parameter([], 'list of preprocessor flags such as -I or -D')
   main_file_only = Parameter(True, 'should only main file be processed')
   cpp_output = Parameter(None, 'filename for preprocessed file')
   base_path = Parameter(None, 'path prefix to strip off of the filenames')
   language = Parameter('C', 'source code programming language of the given input file')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = ast

      flags = self.flags
      base_path = self.base_path and os.path.abspath(self.base_path) + os.sep or ''
      if not self.preprocessor:
         import ucpp
         info = self.get_compiler_info(self.emulate_compiler)
         flags += map(lambda x:'-I%s'%x, info.include_paths)
         flags += map(lambda x:'-D%s=%s'%(x[0], x[1]), info.macros)
         for file in self.input:
            self.ast = ucpp.parse(self.ast,
                                  os.path.abspath(file),
                                  base_path,
                                  self.cpp_output,
                                  self.language, flags, self.main_file_only,
                                  self.verbose, self.debug)
      else:
         print 'not implemented yet: spawn external preprocessor'
      return self.output_and_return_ast()

   def get_compiler_info(self, compiler):
      import emul
      return emul.get_compiler_info(compiler)
