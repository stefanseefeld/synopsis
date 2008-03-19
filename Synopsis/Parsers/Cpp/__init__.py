#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Preprocessor for C, C++, IDL"""

from Synopsis.Processor import *
from Emulator import get_compiler_info
from ParserImpl import parse
import os.path

class Parser(Processor):

    emulate_compiler = Parameter('', 'a compiler to emulate')
    compiler_flags = Parameter([], 'list of flags for the emulated compiler')
    flags = Parameter([], 'list of preprocessor flags such as -I or -D')
    primary_file_only = Parameter(True, 'should only primary file be processed')
    cpp_output = Parameter(None, 'filename for preprocessed file')
    base_path = Parameter(None, 'path prefix to strip off of the filenames')
    language = Parameter('C++', 'source code programming language of the given input file')
    
    def probe(self, **kwds):

        self.set_parameters(kwds)
        if type(self.compiler_flags) != list:
            raise InvalidArgument('compiler_flags=%s (expected list)'%repr(self.compiler_flags))
        return get_compiler_info(self.language,
                                 self.emulate_compiler,
                                 self.compiler_flags)

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        if not self.input: raise MissingArgument('input')
        self.ir = ir

        system_flags = []
        # Accept either a string or a list.
        flags = type(self.flags) is str and self.flags.split() or self.flags
        base_path = self.base_path and os.path.abspath(self.base_path) + os.sep or ''
        info = get_compiler_info(self.language,
                                 self.emulate_compiler,
                                 self.compiler_flags)
        system_flags += ['-I%s'%x for x in info.include_paths]
        system_flags += ['-D%s'%k + (v and '=%s'%v or '') 
                         for (k,v) in info.macros]
        for file in self.input:
            self.ir = parse(self.ir,
                            os.path.abspath(file),
                            base_path,
                            self.cpp_output,
                            self.language, system_flags, flags,
                            self.primary_file_only,
                            self.verbose, self.debug, self.profile)
        return self.output_and_return_ir()

