# $Id: Type.py,v 1.12 2003/11/11 06:05:03 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import string
import Util

def ccmp(a,b):
   "Compares classes of two objects"
   return cmp(type(a),type(b)) or cmp(a.__class__,b.__class__)

class Error:
   """Exception class used by Type internals."""

   def __init__(self, err):
      self.err = err
   def __repr__(self):
      return self.err

class Type:
   """Type abstract class."""

   def __init__(self, language):
      self.__language = language
   def language(self):
      """the language this type was defined in"""
      return self.__language
   def accept(self, visitor):
      """visitor pattern accept. @see Visitor"""
      pass

   def __cmp__(self, other):
      "Comparison operator"
      return cmp(id(self),id(other))

class Named(Type):
   """Named type abstract class"""
   def __init__(self, language, name):
      Type.__init__(self, language)
      if type(name) != type(()) and type(name) != type([]):
         raise TypeError,"Name must be scoped"
      self.__name = tuple(name)
   def name(self):
      """Returns the name of this type as a scoped tuple"""
      return self.__name
   def set_name(self, name):
      """Changes the name of this type"""
      self.__name = name

class Base(Named):
   """Class for base types"""

   def __init__(self, language, name):
      Named.__init__(self, language, name)
   def accept(self, visitor): visitor.visitBaseType(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name(),other.name())
   def __str__(self): return Util.ccolonName(self.name())

class Dependent(Named):
   """Class for template dependent types"""

   def __init__(self, language, name):
      Named.__init__(self, language, name)
   def accept(self, visitor): visitor.visitDependent(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name(),other.name())
   def __str__(self): return Util.ccolonName(self.name())

class Unknown(Named):
   """Class for not (yet) known type"""
   base = Type
   def __init__(self, language, name):
      Named.__init__(self, language, name)
      self.__link = name
   def link(self):
      """external reference this type may be associated with"""
      return self.__link
   def resolve(self, language, name, link):
      """associate this type with an external reference, instead of a declaration"""
      self.base.__language = language
      self.__name = name
      self.__link = link
   def accept(self, visitor): visitor.visitUnknown(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name(),other.name())
   def __str__(self): return Util.ccolonName(self.name())

class Declared(Named):
   """Class for declared types"""

   def __init__(self, language, name, declaration):
      Named.__init__(self, language, name)
      self.__declaration = declaration
   def declaration(self):
      """declaration object which corresponds to this type"""
      return self.__declaration
   def accept(self, visitor): visitor.visitDeclared(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name(),other.name())
   def __str__(self): return Util.ccolonName(self.name())

class Template (Declared):
   """Class for declared parametrized types"""

   def __init__(self, language, name, declaration, parameters):
      Declared.__init__(self, language, name, declaration)
      self.__parameters = parameters
   def parameters(self):
      """list of type names used to declare this template"""
      return self.__parameters
   def accept(self, visitor): visitor.visitTemplate(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.parameters(),other.parameters())
   def __str__(self):
      return "template<%s>%s"%(string.join(self.__parameters,','),
                               Util.ccolonName(self.name()))

class Modifier(Type):
   """Class for alias types with modifiers (such as 'const', '&', etc.)"""

   def __init__(self, language, alias, premod, postmod):
      Type.__init__(self, language)
      self.__alias = alias
      self.__premod = premod
      self.__postmod = postmod
   def alias(self):
      """the type this type refers to"""
      return self.__alias
   def premod(self):
      """the modifier string"""
      return self.__premod
   def postmod(self):
      """the modifier string"""
      return self.__postmod
   def accept(self, visitor): visitor.visitModifier(self)

   def set_alias(self, alias): self.__alias = alias
   def __cmp__(self, other):
      "Comparison operator"
      return (ccmp(self,other)
              or cmp(self.alias(),other.alias())
              or cmp(self.premod(),other.premod())
              or cmp(self.postmod(),other.postmod()))
   def __str__(self): 
      return "%s%s%s"%(string.join(map(lambda s:s+" ",self.__premod),''),
                       str(self.__alias),
                       string.join(self.__postmod,''))

class Array (Type):
   """a modifier that adds array dimensions to a type"""
    
   def __init__(self, language, alias, sizes):
      Type.__init__(self, language)
      self.__alias = alias
      self.__sizes = sizes
   def alias(self): return self.__alias
   """primary type this array decorates"""
   def sizes(self): return self.__sizes
   """dimensions of the array"""
   def accept(self, visitor): visitor.visitArray(self)
   def set_alias(self, alias): self.__alias = alias
   def __cmp__(self, other):
      "Comparison operator"
      return (ccmp(self,other)
              or cmp(self.alias(),other.alias())
              or cmp(self.sizes(),other.sizes()))
   def __str__(self): 
      return "%s%s"%(str(self.__alias),
                     string.join(map(lambda s:"[" + s + "]",self.__sizes),''))

class Parametrized(Type):
   """Class for parametrized type instances"""

   def __init__(self, language, template, parameters):
      Type.__init__(self, language)
      self.__template = template
      self.__parameters = parameters
   def template(self):
      """template type this is an instance of"""
      return self.__template
   def parameters(self):
      """list of types for which this template is instanciated"""
      return self.__parameters
   def accept(self, visitor): visitor.visitParametrized(self)
   def set_template(self, type): self.__template = type
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.template(),other.template())
   def __str__(self):
      return "%s<%s>"%(self.__template.name(),
                       string.join(map(str,self.__parameters),','))

class Function(Type):
   """Class for function pointer types."""
   def __init__(self, language, retType, premod, params):
      Type.__init__(self, language)
      self.__returnType = retType
      self.__premod = premod
      self.__params = params
   def returnType(self):
      """nested return type"""
      return self.__returnType
   def premod(self):
      """list of premodifier strings"""
      return self.__premod
   def parameters(self):
      """list of nested parameter types"""
      return self.__params
   def accept(self, visitor): visitor.visitFunctionType(self)
   
   def set_returnType(self, returnType): self.__returnType = returnType

class Dictionary:
    
   def __init__(self): self.__dict = {}
   def __setitem__(self, name, type): self.__dict[tuple(name)] = type
   def __getitem__(self, name): return self.__dict[tuple(name)]
   def __delitem__(self, name): del self.__dict[tuple(name)]
   def has_key(self, name): return self.__dict.has_key(tuple(name))
   def keys(self): return self.__dict.keys()
   def values(self): return self.__dict.values()
   def items(self): return self.__dict.items()
   def lookup(self, name, scopes):
      """locate 'name' in one of the scopes"""
      for s in scopes:
         scope = list(s)
         while len(scope) > 0:
            if self.has_key(scope + name):
               return self[scope + name]
            else: del scope[-1]
      if self.has_key(name):
         return self[name]
      return None
   def clear(self): self.__dict.clear()
   def merge(self, dict):
      for i in dict.keys():
         if self.has_key(i): 
            if isinstance(self[i], Unknown):
               self[i] = dict[i]
            else:
               pass
         else: self[i] = dict[i]

class Visitor:
   """Visitor for Type objects"""
   def visitBaseType(self, type): return
   def visitUnknown(self, type): return
   def visitDeclared(self, type): return
   def visitModifier(self, type): return
   def visitArray(self, type): return
   def visitTemplate(self, type): return
   def visitParametrized(self, type): return
   def visitFunctionType(self, type): return
   def visitDependent(self, type): return
