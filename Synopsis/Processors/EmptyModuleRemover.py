# $Id: EmptyModuleRemover.py,v 1.5 2003/11/25 20:19:50 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util

import string

class EmptyNS (Processor, AST.Visitor):
   """A class that removes empty namespaces"""

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      self.__scopestack = []
      self.__currscope = []

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
      module.declarations()[:] = self.__currscope
      # count the number of non-forward declarations in the current scope
      count = reduce(lambda x, y: x + y,
                     map(lambda x: not isinstance(x, AST.Forward),
                         self.__currscope), 0)

      if count: self.pop(module)
      else: self.pop_only()
