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

def find_imported(target, root, current, verbose = False):
    """Lookup imported files, based on current file's location.

      target: Pair of names.
      root: root directory to which to confine the lookup.
      current: file name of the module issuing the import."""

    current = os.path.dirname(current)
    if root:
        locations = [current.rsplit(i, 1)[0] for i in current.count('.')]
    else:
        locations = [current]
    modname, name = target
    if name:
        # name may be a module or a module's attribute.
        # Only find the module.
        files = ['%s/%s.py'%(modname, name), '%s.py'%modname]
    else:
        files = ['%s.py'%modname]
    files.append(os.path.join(modname, '__init__.py'))

    for l in locations:
        for f in files:
            target = os.path.join(l, f)
            if verbose: print 'trying %s'%target
            if os.path.exists(target):
                return target, target[len(root):]
    return None, None
    


class Parser(Processor):
    """Python Parser. See http://www.python.org/dev/peps/pep-0258 for additional
    info."""

    primary_file_only = Parameter(True, 'should only primary file be processed')
    base_path = Parameter('', 'Path prefix to strip off of input file names.')
    sxr_prefix = Parameter(None, 'Path prefix (directory) to contain sxr info.')
    
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
        sourcefile = SourceFile(short_filename, long_filename, 'Python', True)
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
                qname = QName(package_name)
                if not os.path.isfile(os.path.join(package_path, '__init__.py')):
                    raise InvalidArgument('"%s" is not a package'%qname)
                module = ASG.Module(sourcefile, -1, 'package', qname)
                self.ir.types[qname] = ASG.Declared('Python', qname, module)
                if package:
                    package.declarations.append(module)
                else:
                    self.ir.declarations.append(module)

                package = module

        translator = ASGTranslator(package, self.ir.types)
        translator.process_file(sourcefile)
        # At this point, sourcefile contains a single declaration: the module.
        if package:
            package.declarations.extend(sourcefile.declarations)
        else:
            self.ir.declarations.extend(sourcefile.declarations)
        if not self.primary_file_only:
            for i in translator.imports:
                target = find_imported(i, self.base_path, sourcefile.name, self.verbose)
                if target[0] and target[1] not in self.ir.files:
                    # Only process if we have not visited it yet.
                    self.all_files.append(target[0])

        if self.sxr_prefix:
            sxr = os.path.join(self.sxr_prefix, short_filename + '.sxr')
            dirname = os.path.dirname(sxr)
            if not os.path.exists(dirname):
                os.makedirs(dirname, 0755)

            sxr_generator = SXRGenerator()
            module = sourcefile.declarations[0]
            sxr_generator.process_file(module.name, sourcefile, sxr)
