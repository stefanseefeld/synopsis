# $Id: AccessRestrictor.py,v 1.2 2003/11/11 02:57:57 stefan Exp $
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

class AccessRestrictor(Processor, AST.Visitor):
   """This class processes declarations, and removes those that need greated
   access than the maximum passed to the constructor"""

   access = Parameter(None, 'specify up to which accessibility level the interface should be documented')

   def __init__(self):

      self.__scopestack = []
      self.__currscope = []

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      if self.access is not None:

         for decl in ast.declarations():
            decl.accept(self)
         ast.declarations()[:] = self.__currscope

      return self.output_and_return_ast()

   def push(self):

      self.__scopestack.append(self.__currscope)
      self.__currscope = []

   def pop(self, decl):

      self.__currscope = self.__scopestack.pop()
      self.__currscope.append(decl)

   def add(self, decl):

      self.__currscope.append(decl)

   def visitDeclaration(self, decl):

      if decl.accessibility() > self.access: return
      self.add(decl)

   def visitScope(self, scope):

      if scope.accessibility() > self.access: return
      self.push()
      for decl in scope.declarations():
         decl.accept(self)
      scope.declarations()[:] = self.__currscope
      self.pop(scope)

linkerOperation = AccessRestrictor
