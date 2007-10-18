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
import ParserImpl

import os, os.path, tempfile

class Parser(Processor):

   preprocess = Parameter(True, 'whether or not to preprocess the input')
   emulate_compiler = Parameter('cc', 'a compiler to emulate')
   cppflags = Parameter([], 'list of preprocessor flags such as -I or -D')
   primary_file_only = Parameter(True, 'should only primary file be processed')
   base_path = Parameter('', 'path prefix to strip off of the file names')
   syntax_prefix = Parameter(None, 'path prefix (directory) to contain syntax info')
   xref_prefix = Parameter(None, 'path prefix (directory) to contain xref info')

   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      self.ir = ir

      if self.preprocess:

         from Synopsis.Parsers import Cpp
         cpp = Cpp.Parser(base_path = self.base_path,
                          language = 'C',
                          flags = self.cppflags,
                          emulate_compiler = self.emulate_compiler)

      base_path = self.base_path and os.path.abspath(self.base_path) + os.sep or ''

      for file in self.input:

         i_file = file
         if self.preprocess:

            if self.output:
               i_file = os.path.splitext(self.output)[0] + '.i'
            else:
               i_file = os.path.join(tempfile.gettempdir(),
                                     'synopsis-%s.i'%os.getpid())
            self.ir = cpp.process(self.ir,
                                  cpp_output = i_file,
                                  input = [file],
                                  primary_file_only = self.primary_file_only,
                                  verbose = self.verbose,
                                  debug = self.debug)

         self.ir = ParserImpl.parse(self.ir, i_file,
                                    os.path.abspath(file),
                                    self.primary_file_only,
                                    base_path,
                                    self.syntax_prefix,
                                    self.xref_prefix,
                                    self.verbose,
                                    self.debug)

         if self.preprocess: os.remove(i_file)

      return self.output_and_return_ir()
