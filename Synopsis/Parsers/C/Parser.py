#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""C Parser
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
import ctool

import os, os.path

class Parser(Processor):

   preprocess = Parameter(True, 'whether or not to preprocess the input')
   emulate_compiler = Parameter('cc', 'a compiler to emulate')
   cppflags = Parameter([], 'list of preprocessor flags such as -I or -D')
   main_file_only = Parameter(True, 'should only main file be processed')
   base_path = Parameter('', 'path prefix to strip off of the file names')
   syntax_prefix = Parameter(None, 'path prefix (directory) to contain syntax info')
   xref_prefix = Parameter(None, 'path prefix (directory) to contain xref info')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = ast

      if self.preprocess:

         from Synopsis.Parsers import Cpp
         cpp = Cpp.Parser(prefix = self.base_path,
                          language = 'C++',
                          flags = self.cppflags,
                          emulate_compiler = self.emulate_compiler)

      for file in self.input:

         if self.preprocess:

            stem = os.path.splitext(self.output)[0]
            i_file = stem + '.i'
            self.ast = cpp.process(self.ast,
                                   cpp_output = i_file,
                                   input = [file])

            self.ast = ctool.parse(self.ast, i_file,
                                   file,
                                   self.base_path,
                                   self.syntax_prefix,
                                   self.xref_prefix,
                                   self.verbose,
                                   self.debug)

      return self.output_and_return_ast()
