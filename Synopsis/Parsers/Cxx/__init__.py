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

from Synopsis.Processor import *
import ParserImpl

import os, os.path, tempfile

class Parser(Processor):

    preprocess = Parameter(True, 'whether or not to preprocess the input')
    emulate_compiler = Parameter('', 'a compiler to emulate')
    compiler_flags = Parameter([], 'list of flags for the emulated compiler')
    cppflags = Parameter([], 'list of preprocessor flags such as -I or -D')
    primary_file_only = Parameter(True, 'should only primary file be processed')
    base_path = Parameter('', 'path prefix to strip off of the file names')
    sxr_prefix = Parameter(None, 'path prefix (directory) to contain sxr info')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        if not self.input: raise MissingArgument('input')
        self.ir = ir

        if self.preprocess:

            from Synopsis.Parsers import Cpp
            cpp = Cpp.Parser(base_path = self.base_path,
                             language = 'C++',
                             flags = self.cppflags,
                             emulate_compiler = self.emulate_compiler,
                             compiler_flags = self.compiler_flags)

        for file in self.input:

            ii_file = file
            if self.preprocess:

                if self.output:
                    ii_file = os.path.splitext(self.output)[0] + '.ii'
                else:
                    ii_file = os.path.join(tempfile.gettempdir(),
                                           'synopsis-%s.ii'%os.getpid())
                self.ir = cpp.process(self.ir,
                                      cpp_output = ii_file,
                                      input = [file],
                                      primary_file_only = self.primary_file_only,
                                      verbose = self.verbose,
                                      debug = self.debug)

            try:
                self.ir = ParserImpl.parse(self.ir, ii_file,
                                           os.path.abspath(file),
                                           self.primary_file_only,
                                           os.path.abspath(self.base_path) + os.sep,
                                           self.sxr_prefix,
                                           self.verbose,
                                           self.debug)
            finally:
                if self.preprocess: os.remove(ii_file)

            return self.output_and_return_ir()
