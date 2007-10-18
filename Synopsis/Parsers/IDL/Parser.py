#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Parser for IDL using omniidl for low-level parsing."""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
import omni
import os, os.path, tempfile

class Parser(Processor):

    preprocess = Parameter(True, 'whether or not to preprocess the input')
    cppflags = Parameter([], 'list of preprocessor flags such as -I or -D')
    primary_file_only = Parameter(True, 'should only primary file be processed')
    base_path = Parameter('', 'path prefix to strip off of the file names')
   
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = ir

        if self.preprocess:

            from Synopsis.Parsers import Cpp
            cpp = Cpp.Parser(base_path = self.base_path,
                             language = 'IDL',
                             flags = self.cppflags,
                             emulate_compiler = None)

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


            self.ir = omni.parse(self.ir, i_file,
                                 os.path.abspath(file),
                                 self.primary_file_only,
                                 os.path.abspath(self.base_path) + os.sep,
                                 self.verbose,
                                 self.debug)

            if self.preprocess: os.remove(i_file)

        return self.output_and_return_ir()

