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

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
import occ
import os

class Parser(Processor):

   preprocessor = Parameter(None, 'the preprocessor to use (defaults to internal)')
   preprocess = Parameter(True, 'whether or not to preprocess the input')
   emulate_compiler = Parameter('c++', 'a compiler to emulate (defaults to \'c++\')')
   cppflags = Parameter([], 'list of preprocessor flags such as -I or -D')
   main_file_only = Parameter(True, 'should only main file be processed')
   base_path = Parameter('', 'path prefix to strip off of the file names')
   syntax_prefix = Parameter(None, 'path prefix (directory) to contain syntax info')
   xref_prefix = Parameter(None, 'path prefix (directory) to contain xref info')

   extra_files = Parameter([], 'extra files for which to keep info. Highly Deprecated !!!')
   
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

         ii_file = file
         if self.preprocess:

            ii_file = os.path.splitext(self.output)[0] + '.ii'
            self.ast = cpp.process(self.ast,
                                   cpp_output = ii_file,
                                   input = [file],
                                   verbose = self.verbose,
                                   debug = self.debug)

         self.ast = occ.parse(self.ast, ii_file, file,
                              self.extra_files,
                              self.verbose,
                              self.main_file_only,
                              self.base_path,
                              self.syntax_prefix,
                              self.xref_prefix)

         if self.preprocess: os.remove(ii_file)

      return self.output_and_return_ast()
