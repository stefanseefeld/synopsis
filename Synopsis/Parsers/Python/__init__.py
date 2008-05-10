#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
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

def expand_package(root, verbose = False):
    """Find all modules in a given package."""

    modules = []
    if not os.path.isdir(root) or not os.path.isfile(os.path.join(root, '__init__.py')):
        return modules
    if verbose: print 'expanding %s'%root
    files = os.listdir(root)
    modules += [os.path.join(root, file)
                for file in files if os.path.splitext(file)[1] == '.py']
    for d in [dir for dir in files if os.path.isdir(os.path.join(root, dir))]:
        modules.extend(expand_package(os.path.join(root, d), verbose))
    return modules


def find_imported(target, base_path, origin, verbose = False):
    """
    Lookup imported files, based on current file's location.

    target: (module, name) pair.
    base_path: root directory to which to confine the lookup.
    origin: file name of the module issuing the import."""

    module, name = target[0].replace('.', os.sep), target[1]
    origin = os.path.dirname(origin)
    if base_path:
        locations = [base_path + origin.rsplit(os.sep, i)[0]
                     for i in range(origin.count(os.sep) + 1)] + [base_path]
    else:
        locations = [origin]

    # '*' is only valid in a module scope
    if name and name != '*':
        # name may be a module or a module's attribute.
        # Only find the module.
        files = ['%s/%s.py'%(module, name), '%s.py'%module]
    else:
        files = ['%s.py'%module]
    files.append(os.path.join(module, '__init__.py'))
    for l in locations:
        for f in files:
            target = os.path.join(l, f)
            if verbose: print 'trying %s'%target
            if os.path.exists(target):
                return target, target[len(base_path):]
    return None, None
    


class Parser(Processor):
    """
    Python Parser. See http://www.python.org/dev/peps/pep-0258 for additional
    info."""

    primary_file_only = Parameter(True, 'should only primary file be processed')
    base_path = Parameter(None, 'Path prefix to strip off of input file names.')
    sxr_prefix = Parameter(None, 'Path prefix (directory) to contain sxr info.')
    default_docformat = Parameter('', 'default documentation format')
    
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        if not self.input: raise MissingArgument('input')
        self.ir = ir
        self.scopes = []
      
        # Create return type for Python functions:
        self.return_type = ASG.BuiltinTypeId('Python',('',))

        # Validate base_path.
        if self.base_path:
            if not os.path.isdir(self.base_path):
                raise InvalidArgument('base_path: "%s" not a directory.'
                                      %self.base_path)
            if self.base_path[-1] != os.sep:
                self.base_path += os.sep

        # all_files is a list of (filename, base_path) pairs.
        # we have to record the base_path per file, since it defaults to the
        # filename itself, for files coming directly from user input.
        # For files that are the result of expanded packages it defaults to the
        # package directory itself.
        self.all_files = []
        for i in self.input:
            base_path = self.base_path or os.path.dirname(i)
            if base_path and base_path[-1] != os.sep:
                base_path += os.sep
            # expand packages into modules
            if os.path.isdir(i):
                if os.path.exists(os.path.join(i, '__init__.py')):
                    self.all_files.extend([(p,base_path)
                                           for p in expand_package(i, self.verbose)])
                else:
                    raise InvalidArgument('"%s" is not a Python package'%i)
            else:
                self.all_files.append((i,base_path))
        # process modules
        while len(self.all_files):
            file, base_path = self.all_files.pop()
            self.process_file(file, base_path)

        return self.output_and_return_ir()


    def process_file(self, filename, base_path):
        """Parse an individual python file."""

        long_filename = filename
        if filename[:len(base_path)] != base_path:
            raise InvalidArgument('invalid input filename:\n'
                                  '"%s" does not match base_path "%s"'
                                  %(filename, base_path))
        if self.verbose: print 'parsing %s'%filename
        short_filename = filename[len(base_path):]
        sourcefile = SourceFile(short_filename, long_filename, 'Python', True)
        self.ir.files[short_filename] = sourcefile

        package = None
        package_name = []
        package_path = base_path
        # Only attempt to set up enclosing packages if a base_path was given.
        if package_path != filename:
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
                # Try to locate the package
                type_id = self.ir.asg.types.get(qname)
                if (type_id):
                    module = type_id.declaration
                else:
                    module = ASG.Module(sourcefile, -1, 'package', qname)
                    self.ir.asg.types[qname] = ASG.DeclaredTypeId('Python', qname, module)
                    if package:
                        package.declarations.append(module)
                    else:
                        self.ir.asg.declarations.append(module)

                package = module

        translator = ASGTranslator(package, self.ir.asg.types, self.default_docformat)
        translator.process_file(sourcefile)
        # At this point, sourcefile contains a single declaration: the module.
        if package:
            package.declarations.extend(sourcefile.declarations)
        else:
            self.ir.asg.declarations.extend(sourcefile.declarations)
        if not self.primary_file_only:
            for i in translator.imports:
                target = find_imported(i, self.base_path, sourcefile.name, self.verbose)
                if target[0] and target[1] not in self.ir.files:
                    # Only process if we have not visited it yet.
                    self.all_files.append((target[0], base_path))

        if self.sxr_prefix:
            sxr = os.path.join(self.sxr_prefix, short_filename + '.sxr')
            dirname = os.path.dirname(sxr)
            if not os.path.exists(dirname):
                os.makedirs(dirname, 0755)

            sxr_generator = SXRGenerator()
            module = sourcefile.declarations[0]
            sxr_generator.process_file(module.name, sourcefile, sxr)
