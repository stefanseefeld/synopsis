# $Id: Stripper.py,v 1.8 2003/11/21 21:18:36 stefan Exp $
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

class Stripper(Processor, AST.Visitor):
   """Strip common prefix from the declaration's name.
   Keep a list of root nodes, such that children whos parent
   scopes are not accepted but which themselfs are correct can
   be maintained as new root nodes."""

   scope = Parameter([], 'strip all but the given scope')

   def __init__(self):
      
      self.__root = []
      self.__in = 0

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)
      if self.scope:

         #self.__scope = map(lambda x: tuple(string.split(x, "::")), self.scope)
         self.__scope = tuple(string.split(self.scope, "::"))
         # strip prefixes and remove non-matching declarations
         self.strip_declarations(self.ast.declarations())

         # Remove types not in strip
         self.strip_types(self.ast.types())

      return self.output_and_return_ast()
      
   def strip_name(self, name):
      #for scope in self.__scopes:
      depth = len(self.scope)
      if name[0:depth] == self.scope:
         if len(name) == depth: return None
         return name[depth:]
      return None
    
   def strip_declarations(self, declarations):
      for decl in declarations:
         decl.accept(self)
      declarations[:] = self.declarations()
	
   def strip_types(self, types):
      # Remove the empty type (caused by C++ with extract_tails)
      if types.has_key(()): del types[()]
      for name, type in types.items():
         try:
            del types[name]
            name = self.strip_name(name)
            if name:
               type.set_name(name)
               types[name] = type
         except:
            print "ERROR Processing:", name, types[name]
            raise

   def declarations(self): return self.__root
   
   def strip(self, declaration):
      """test whether the declaration matches one of the prefixes, strip
      it off, and return success. Success means that the declaration matches
      the prefix set and thus should not be removed from the AST."""
      passed = 0
      if not self.__scope: return 1
      for scope in [self.__scope]:
         depth = len(scope)
         name = declaration.name()
         if name[0:depth] == scope:
            if len(name) == depth: break
            if self.verbose: print "symbol", string.join(name, "::"),
            declaration.set_name(name[depth:])
            if self.verbose: print "stripped to", string.join(name, "::")
            passed = 1
         else: break
      if self.verbose and not passed:
         print "symbol", string.join(declaration.name(), "::"), "removed"
      return passed

   def visitScope(self, scope):
      root = self.strip(scope) and not self.__in
      if root:
         self.__in = 1
         self.__root.append(scope)
      for declaration in scope.declarations():
         declaration.accept(self)
      if root: self.__in = 0

   def visitClass(self, clas):
      self.visitScope(clas)
      templ = clas.template()
      if templ:
         name = self.strip_name(templ.name())
         if name: templ.set_name(name)

   def visitDeclaration(self, decl):
      if self.strip(decl) and not self.__in:
         self.__root.append(decl)

   def visitEnumerator(self, enumerator):
      self.strip(enumerator)

   def visitEnum(self, enum):
      self.visitDeclaration(enum)
      for e in enum.enumerators():
         e.accept(self)

   def visitFunction(self, function):
      self.visitDeclaration(function)
      for parameter in function.parameters():
         parameter.accept(self)
      templ = function.template()
      if templ:
         name = self.strip_name(templ.name())
         if name: templ.set_name(name)

   def visitOperation(self, operation):
      self.visitFunction(operation)

   def visitMetaModule(self, module):
      self.visitScope(module)
      for decl in module.module_declarations():
         name = self.strip_name(decl.name())
         if name: decl.set_name(name)

linkerOperation = Stripper
