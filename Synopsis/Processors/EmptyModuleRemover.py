# $Id: EmptyModuleRemover.py,v 1.3 2003/11/11 04:51:17 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis.Core import AST, Type, Util

import string

class EmptyNS (Processor, AST.Visitor):
   """A class that removes empty namespaces"""

   def __init__(self):

      self.__scopestack = []
      self.__currscope = []

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      for decl in self.ast.declarations():
         decl.accept(self)
      self.ast.declarations()[:] = self.__currscope

      return self.output_and_return_ast()

   def push(self):
      """Pushes the current scope onto the stack and starts a new one"""

      self.__scopestack.append(self.__currscope)
      self.__currscope = []

   def pop(self, decl):
      """Pops the current scope from the stack, and appends the given
      declaration to it"""

      self.__currscope = self.__scopestack.pop()
      self.__currscope.append(decl)

   def pop_only(self):
      """Only pops, doesn't append to scope"""

      self.__currscope = self.__scopestack.pop()

   def add(self, decl):
      """Adds the given decl to the current scope"""
      self.__currscope.append(decl)

   def currscope(self):
      """Returns the current scope: a list of declarations"""

      return self.__currscope

   def visitDeclaration(self, decl):
      """Adds declaration to scope"""
      self.add(decl)

   def visitGroup(self, group):
      """Overrides recursive behaviour to just add the group"""

      self.add(group)

   def visitEnum(self, enum):
      """Overrides recursive behaviour to just add the enum"""

      self.add(enum)

   def visitModule(self, module):
      """Visits all children of the module, and if there are no declarations
      after that removes the module"""
      self.push()
      for decl in module.declarations():
         decl.accept(self)
      module.declarations()[:] = self.currscope()
      count = self._count_not_forwards(self.currscope())
      if count: self.pop(module)
      else: self.pop_only()

   def _count_not_forwards(self, decls):
      """Returns the number of declarations not instances of AST.Forward"""

      count = 0
      for decl in decls:
         if not isinstance(decl, AST.Forward): count = count+1
      return count

linkerOperation = EmptyNS
