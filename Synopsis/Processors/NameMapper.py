# $Id: NameMapper.py,v 1.7 2003/11/20 04:46:38 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import string

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util

class NameMapper(Processor, AST.Visitor):
   """Abstract base class for name mapping."""

   def visitGroup(self, node):
      """Recursively visits declarations under this group/scope/etc"""

      self.visitDeclaration(node)
      for declaration in node.declarations():
         declaration.accept(self)   

class NamePrefixer(NameMapper):
   """This class adds a prefix to all declaration and type names."""

   prefix = Parameter([], 'the prefix which to prepend to all declared types')
   type = Parameter('Language', 'type to use for the new toplevel modules')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      if not self.prefix:
         return self.output_and_return_ast()

      for decl in self.ast.declarations():
         decl.accept(self)

      # Now we need to put the declarations in actual nested MetaModules
      lang, type = '', self.type
      for index in range(len(self.prefix), 0, -1):
         module = AST.MetaModule(lang, type, self.prefix[:index])
         module.declarations().extend(self.ast.declarations())
         self.ast.types()[module.name()] = Type.Declared(lang,
                                                         module.name(),
                                                         module)
         self.ast.declarations()[:] = [module]

      return self.output_and_return_ast()

   def visitDeclaration(self, decl):
      """Changes the name of this declaration and its associated type"""

      # Change the name of the decl
      name = decl.name()
      new_name = tuple(self.prefix + list(name))
      decl.set_name(new_name)
      # Change the name of the associated type
      try:
         type = self.ast.types()[name]
         del self.ast.types()[name]
         type.set_name(new_name)
         self.ast.types()[new_name] = type
      except KeyError, msg:
         if self.verbose: print "Warning: Unable to map name of type:",msg

