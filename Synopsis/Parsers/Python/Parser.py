#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
"""Main module for the Python parser.
Parsing python is achieved by using the code in the Python distribution that
is an example for parsing python by using the built-in parser. This parser
returns a parse tree which we can traverse and translate into Synopsis' AST.
The exparse module contains the enhanced example code (it has many more
features than the simple example did), and this module translates the
resulting intermediate AST objects into Synopsis.Core.AST objects.
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type
from Synopsis.SourceFile import SourceFile
from Synopsis.DocString import DocString

import parser, exparse, sys, os, string, getopt

class Parser(Processor):

   base_path = Parameter('', 'path prefix to strip off of the file names')
    
   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = ast
      self.scopes = []
      
      # Create return type for Python functions:
      self.return_type = Type.Base('Python',('',))
      
      for file in self.input:
         self.process_file(file)

      return self.output_and_return_ast()

   def process_file(self, file):
      """Entry point for the Python parser"""

      filename = file
      types = self.ast.types()

      # Split the filename into a scoped tuple name
      name = file
      if file[:len(self.base_path)] == self.base_path:
         name = filename = file[len(self.base_path):]
      name = string.split(os.path.splitext(name)[0], os.sep)
      exparse.packagepath = string.join(string.split(file, os.sep)[:-1], os.sep)
      exparse.packagename = name[:-1]
      #print "package path, name:",exparse.packagepath, exparse.packagename
      self.sourcefile = SourceFile(filename, file, 'Python')
      self.sourcefile.annotations['primary'] = True

      # Create the enclosing module scopes
      self.scopes = [AST.Scope(self.sourcefile, -1, 'file scope', name)]
      for depth in range(len(name) - 1):
         module = AST.Module(self.sourcefile, -1, 'package', name[:depth+1])
         self.add_declaration(module)
         types[module.name()] = Type.Declared('Python', module.name(), module)
         self.push(module)
      # Add final name as Module -- unless module is __init__
      if name[-1] != '__init__':
         module = AST.Module(self.sourcefile, -1, 'module', name)
         self.add_declaration(module)
         types[module.name()] = Type.Declared('Python', module.name(), module)
         self.push(module)
    
      # Parse the file
      mi = exparse.get_docs(file)

      # Process the parsed file
      self.process_module_info(mi)

      # Store the declarations
      self.ast.files()[filename] = self.sourcefile
      self.ast.declarations().extend(self.scopes[0].declarations())

   def add_declaration(self, decl):
      """Adds the given declaration to the current top scope and to the
      SourceFile for this file."""

      self.scopes[-1].declarations().append(decl)
      self.sourcefile.declarations.append(decl)

   def push(self, scope):
      """Pushes the given scope onto the top of the stack"""

      self.scopes.append(scope)

   def pop(self):
      """Pops the scope stack by one level"""

      del self.scopes[-1]

   def scope_name(self, name):
      """Scopes the given name. If the given name is a list then it is returned
      verbatim, else it is concatenated with the (scoped) name of the current
      scope"""

      if type(name) == type([]): return tuple(name)
      return tuple(list(self.scopes[-1].name()) + [name])

   def process_module_info(self, mi):
      """Processes a ModuleInfo object. The docs are extracted, and any
      functions and docs recursively processed."""

      # TODO: Look up __docformat__ string to figure out markup.
      self.scopes[-1].annotations['doc'] = DocString(mi.get_docstring(), '')
      for name in mi.get_function_names():
         self.process_function_info(mi.get_function_info(name))
      for name in mi.get_class_names():
         self.process_class_info(mi.get_class_info(name))

   def add_params(self, func, fi):
      """Adds the parameters of 'fi' to the AST.Function 'func'."""

      for name, value in map(None, fi.get_params(), fi.get_param_defaults()):
         func.parameters().append(AST.Parameter('', self.return_type,
                                                '', name,value))

   def process_function_info(self, fi):
      """Process a FunctionInfo object. An AST.Function object is created and
      inserted into the current scope."""
      
      name = self.scope_name(fi.get_name())
      func = AST.Function(self.sourcefile, -1, 'function', '',
                          self.return_type, '', name, name[-1])
      func.annotations['doc'] = fi.get_docstring()
      self.add_params(func, fi)
      self.add_declaration(func)

   def process_method_info(self, fi):
      """Process a MethodInfo object. An AST.Operation object is created and
      inserted into the current scope."""

      name = self.scope_name(fi.get_name())
      func = AST.Operation(self.sourcefile,-1, 'operation', '',
                           self.return_type, '', name, name[-1])
      func.annotations['doc'] = fi.get_docstring()
      self.add_params(func, fi)
      self.add_declaration(func)

   def process_class_info(self, ci):
      """Process a ClassInfo object. An AST.Class object is created and
      inserted into the current scope. The inheritance of the class is also
      parsed, and nested classes and methods recursively processed."""

      name = self.scope_name(ci.get_name())
      clas = AST.Class(self.sourcefile,-1, 'class', name)
      clas.annotations['doc'] = ci.get_docstring()

      types = self.ast.types()
      # Figure out bases:
      for base in ci.get_base_names():
         try:
            t = types[self.scope_name(base)]
         except KeyError:
            t = Type.Unknown('Python', self.scope_name(base))
         clas.parents().append(AST.Inheritance('', t, ''))
      # Add the new class
      self.add_declaration(clas)
      types[clas.name()] = Type.Declared('Python', clas.name(), clas)
      self.push(clas)
      for name in ci.get_class_names():
         self.process_class_info(ci.get_class_info(name))
      for name in ci.get_method_names():
         self.process_method_info(ci.get_method_info(name))
      self.pop()
    
   def get_synopsis(self, file):
      """Returns the docstring from the top of an open file"""

      mi = exparse.get_docs(file)
      return mi.get_docstring()

