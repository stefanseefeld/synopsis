#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU GPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import ASG
from Synopsis.QualifiedName import QualifiedPythonName as QName
from Synopsis.SourceFile import SourceFile
from Synopsis.DocString import DocString
from ASGTranslator import ASGTranslator
from SXRGenerator import SXRGenerator
import os

__all__ = ['Parser']

class Parser(Processor):
    """Python Parser. See http://www.python.org/dev/peps/pep-0258 for additional
    info."""

    primary_file_only = Parameter(True, 'should only primary file be processed')
    base_path = Parameter('', 'Path prefix to strip off of input file names.')
    syntax_prefix = Parameter(None, 'Path prefix (directory) to contain syntax info.')
    
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = ir
        self.scopes = []
      
        # Create return type for Python functions:
        self.return_type = ASG.BaseType('Python',('',))

        # Validate base_path.
        if self.base_path:
            if not os.path.isdir(self.base_path):
                raise InvalidArgument('base_path: "%s" not a directory.'
                                      %self.base_path)
            if self.base_path[-1] != os.sep:
                self.base_path += os.sep
            
        self.all_files = self.input[:]
        while len(self.all_files):
            file = self.all_files.pop()
            self.process_file(file)

        return self.output_and_return_ir()


    def process_file(self, filename):
        """Parse an individual python file."""

        long_filename = filename
        if filename[:len(self.base_path)] != self.base_path:
            raise InvalidArguent('invalid input filename:\n'
                                 '"%" does not match base_path "%s"'
                                 %(filename, self.base_path))

        short_filename = filename[len(self.base_path):]
        sourcefile = SourceFile(short_filename, long_filename, 'Python')
        sourcefile.annotations['primary'] = True
        self.ir.files[short_filename] = sourcefile

        package = None
        package_name = []
        package_path = self.base_path
        # Only attempt to set up enclosing packages if a base_path was given.
        if package_path:
            components = short_filename.split(os.sep)
            if components[0] == '':
                package_path += os.sep
                components = components[1:]
            for c in components[:-1]:
                package_name.append(c)
                package_path = os.path.join(package_path, c)
                if not os.path.isfile(os.path.join(package_path, '__init__.py')):
                    raise InvalidArgument('"%s" is not a package'%QName(package_name))
                module = ASG.Module(sourcefile, -1, 'package', QName(package_name))
                if package:
                    package.declarations.append(module)
                else:
                    self.ir.declarations.append(module)

                package = module

        translator = ASGTranslator(package, self.ir.types)
        translator.process_file(sourcefile)
        self.ir.declarations.extend(sourcefile.declarations)
        if not self.primary_file_only:
            for i in translator.imports:
                # Lookup import targets based on current file's location.
                print 'looking up', i

        if self.syntax_prefix:
            xref = os.path.join(self.syntax_prefix, short_filename + '.sxr')
            dirname = os.path.dirname(xref)
            if not os.path.exists(dirname):
                os.makedirs(dirname, 0755)

            sxr_generator = SXRGenerator()
            module = sourcefile.declarations[0]
            sxr_generator.process_file(module.name, sourcefile, xref)
