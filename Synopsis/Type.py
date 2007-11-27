#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

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

class Type(object):
   """Type abstract class."""

   def __init__(self, language):
      self.language = language

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
      self._name = name

   def _set_name(self, name): self._name = tuple(name)
   name = property(lambda self: self._name, _set_name)


class Base(Named):
   """Class for base types"""

   def __init__(self, language, name):
      Named.__init__(self, language, name)
   def accept(self, visitor): visitor.visit_base_type(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name,other.name)
   def __str__(self): return Util.ccolonName(self.name)

class Dependent(Named):
   """Class for template dependent types"""

   def __init__(self, language, name):
      Named.__init__(self, language, name)
   def accept(self, visitor): visitor.visit_dependent(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name,other.name)
   def __str__(self): return Util.ccolonName(self.name)

class Unknown(Named):
   """Class for not (yet) known type"""
   base = Type
   def __init__(self, language, name):
      Named.__init__(self, language, name)
      self.link = name
   def resolve(self, language, name, link):
      """associate this type with an external reference, instead of a declaration"""
      self.base.language = language
      self.name = name
      self.link = link
   def accept(self, visitor): visitor.visit_unknown(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name,other.name)
   def __str__(self): return Util.ccolonName(self.name)

class Declared(Named):
   """Class for declared types"""

   def __init__(self, language, name, declaration):
      Named.__init__(self, language, name)
      self.declaration = declaration

   def accept(self, visitor): visitor.visit_declared(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.name,other.name)
   def __str__(self): return Util.ccolonName(self.name)

class Template(Declared):
   """Class for declared parametrized types"""

   def __init__(self, language, name, declaration, parameters):

      Declared.__init__(self, language, name, declaration)
      self.parameters = parameters

   def accept(self, visitor): visitor.visit_template(self)
   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.parameters,other.parameters)
   def __str__(self):
      return "template<%s>%s"%(','.join(str(self.parameters)),
                               Util.ccolonName(self.name))

class Modifier(Type):
   """Class for alias types with modifiers (such as 'const', '&', etc.)"""

   def __init__(self, language, alias, premod, postmod):
      Type.__init__(self, language)
      self.alias = alias
      self.premod = premod
      self.postmod = postmod

   def accept(self, visitor): visitor.visit_modifier(self)

   def __cmp__(self, other):
      "Comparison operator"
      return (ccmp(self,other)
              or cmp(self.alias,other.alias)
              or cmp(self.premod,other.premod)
              or cmp(self.postmod,other.postmod))
   def __str__(self): 
      return "%s%s%s"%(''.join(['%s '%s for s in self.premod]),
                       str(self.alias),
                       ''.join(self.postmod))

class Array(Type):
   """a modifier that adds array dimensions to a type"""
    
   def __init__(self, language, alias, sizes):
      Type.__init__(self, language)
      self.alias = alias
      self.sizes = sizes
   def accept(self, visitor): visitor.visit_array(self)
   def __cmp__(self, other):
      "Comparison operator"
      return (ccmp(self,other)
              or cmp(self.alias,other.alias)
              or cmp(self.sizes,other.sizes))
   def __str__(self): 
      return "%s%s"%(str(self.alias),
                     ''.join(['[%d]'%s for s in self.sizes]))

class Parametrized(Type):
   """Class for parametrized type instances"""

   def __init__(self, language, template, parameters):

      Type.__init__(self, language)
      self.template = template
      self.parameters = parameters

   def accept(self, visitor): visitor.visit_parametrized(self)

   def __cmp__(self, other):
      "Comparison operator"
      return ccmp(self,other) or cmp(self.template,other.template)
   def __str__(self):
      return "%s<%s>"%('::'.join(self.template.name),
                       ','.join([str(a) for a in self.parameters]))

class Function(Type):
   """Class for function pointer types."""
   def __init__(self, language, return_type, premod, parameters):

      Type.__init__(self, language)
      self.return_type = return_type
      self.premod = premod
      self.parameters = parameters

   def accept(self, visitor): visitor.visit_function_type(self)
   

class Dictionary(dict):
   """Dictionary extends the builtin 'dict' in two ways:
   It allows (modifiable) lists as keys (this is for convenience only, as users
   don't need to convert to tuple themselfs), and it adds a 'lookup' method to
   find a type in a set of scopes."""
   def __setitem__(self, name, type): dict.__setitem__(self, tuple(name), type)
   def __getitem__(self, name): return dict.__getitem__(self, tuple(name))
   def __delitem__(self, name): dict.__delitem__(self, tuple(name))
   def has_key(self, name): return dict.has_key(self, tuple(name))
   def get(self, name, default=None): return dict.get(self, tuple(name), default)
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
   def merge(self, dict):
      """merge in a foreign dictionary, overriding already defined types only
      if they are of type 'Unknown'."""
      for i in dict.keys():
         if self.has_key(i): 
            if isinstance(self[i], Unknown):
               self[i] = dict[i]
            else:
               pass
         else: self[i] = dict[i]

class Visitor(object):
   """Visitor for Type objects"""
   def visit_base_type(self, type): return
   def visit_unknown(self, type): return
   def visit_declared(self, type): return
   def visit_modifier(self, type): return
   def visit_array(self, type): return
   def visit_template(self, type): return
   def visit_parametrized(self, type): return
   def visit_function_type(self, type): return
   def visit_dependent(self, type): return
