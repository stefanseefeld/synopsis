# $Id: NameMapper.py,v 1.4 2003/11/11 02:57:57 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import string

from Synopsis.Processor import Processor, Parameter
from Synopsis.Core import AST, Type, Util

from Linker import config

class NameMapper (Processor, AST.Visitor):
   """This class adds a prefix to all declaration and type names."""

   map_declaration_names = Parameter(None, '')
   map_declaration_type = Parameter('Language', '')

   def visitDeclaration(self, decl):
      """Changes the name of this declaration and its associated type"""

      # Change the name of the decl
      name = decl.name()
      newname = tuple(self.map_declaration_names + name)
      decl.set_name(newname)
      # Change the name of the associated type
      try:
         type = self.types[name]
         del self.types[name]
         type.set_name(newname)
         self.types[newname] = type
      except KeyError, msg:
         if self.verbose: print "Warning: Unable to map name of type:",msg

   def visitGroup(self, node):
      """Recursively visits declarations under this group/scope/etc"""

      self.visitDeclaration(node)
      for declaration in node.declarations():
         declaration.accept(self)

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      if not self.map_declaration_names:
         return self.output_and_return_ast()
      declarations = ast.declarations()
      types = ast.types()
      # Map the names of declarations and their types
      for decl in declarations:
         decl.accept(self)
      # Now we need to put the declarations in actual nested MetaModules
      lang, type = '', self.map_declaration_type
      names = self.map_declaration_names
      for index in range(len(names),0, -1):
         module = AST.MetaModule(lang, type, names[:index])
         module.declarations().extend(declarations)
         types[module.name()] = Type.Declared(lang, module.name(), module)
         declarations[:] = [module]

      return self.output_and_return_ast()

linkerOperation = NameMapper
