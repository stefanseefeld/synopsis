#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
from Emulator import get_compiler_info
from ParserImpl import parse
import os.path

class Parser(Processor):
    """Preprocessor for C, C++, IDL"""

    emulate_compiler = Parameter('', 'a compiler to emulate')
    flags = Parameter([], 'list of preprocessor flags such as -I or -D')
    primary_file_only = Parameter(True, 'should only primary file be processed')
    cpp_output = Parameter(None, 'filename for preprocessed file')
    base_path = Parameter(None, 'path prefix to strip off of the filenames')
    language = Parameter('C++', 'source code programming language of the given input file')

    def process(self, ast, **kwds):

        self.set_parameters(kwds)
        self.ast = ast

        # Accept either a string or a list.
        flags = type(self.flags) is str and self.flags.split() or self.flags
        base_path = self.base_path and os.path.abspath(self.base_path) + os.sep or ''
        if self.emulate_compiler is not None:
            info = get_compiler_info(self.language, self.emulate_compiler)
            system_flags = ['-I%s'%x for x in info.include_paths]
            system_flags += ['-D%s'%k + (v and '=%s'%v or '') for (k,v) in info.macros]
        for file in self.input:
            self.ast = parse(self.ast,
                             os.path.abspath(file),
                             base_path,
                             self.cpp_output,
                             self.language, system_flags, flags,
                             self.primary_file_only,
                             self.verbose, self.debug, self.profile)
        return self.output_and_return_ast()
