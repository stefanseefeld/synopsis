#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Abstract Syntax Tree classes.

This file contains classes which encapsulate nodes in the ASG. The base class
is the Declaration class that encapsulates a named declaration. All names used
are scoped tuples.

Also defined in module scope are the constants DEFAULT, PUBLIC, PROTECTED and
PRIVATE.
"""

import Util


# Accessibility constants
DEFAULT = 0
PUBLIC = 1
PROTECTED = 2
PRIVATE = 3

def ccmp(a,b):
    """Compares classes of two objects"""
    return cmp(type(a),type(b)) or cmp(a.__class__,b.__class__)


class Error:
   """Exception class used by Type internals."""

   def __init__(self, err):
      self.err = err
   def __repr__(self):
      return self.err


class Debugger(type):
    """Wrap the object's 'accept' method, printing out the visitor's type.
    Useful for tracing visitors visiting declarations."""

    def __init__(cls, name, bases, dict):

        accept = dict['accept']
        "The original instancemethod."
       
        def accept_wrapper(self, visitor):
            "The wrapper. The original 'accept' method is part of its closure."
            print '%s accepting %s.%s'%(self.__class__.__name__,
                                        visitor.__module__,
                                        visitor.__class__.__name__)
            accept(self, visitor)

        setattr(cls, 'accept', accept_wrapper)


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

class NamedType(Type):
    """Named type abstract class"""

    def __init__(self, language, name):
        super(NamedType, self).__init__(language)
        self._name = name

    def _set_name(self, name): self._name = tuple(name)
    name = property(lambda self: self._name, _set_name)


class BaseType(NamedType):
    """Class for base types"""

    def __init__(self, language, name):
        super(BaseType, self).__init__(language, name)
    def accept(self, visitor): visitor.visit_base_type(self)
    def __cmp__(self, other):
        "Comparison operator"
        return ccmp(self,other) or cmp(self.name,other.name)
    def __str__(self): return Util.ccolonName(self.name)

class Dependent(NamedType):
    """Class for template dependent types"""

    def __init__(self, language, name):
        super(Dependent, self).__init__(language, name)
    def accept(self, visitor): visitor.visit_dependent(self)
    def __cmp__(self, other):
        "Comparison operator"
        return ccmp(self,other) or cmp(self.name,other.name)
    def __str__(self): return Util.ccolonName(self.name)

class UnknownType(NamedType):
    """Class for not (yet) known type"""
    base = Type
    def __init__(self, language, name):
        super(UnknownType, self).__init__(language, name)
        self.link = name
    def resolve(self, language, name, link):
        """associate this type with an external reference, instead of a declaration"""
        self.base.language = language
        self.name = name
        self.link = link
    def accept(self, visitor): visitor.visit_unknown_type(self)
    def __cmp__(self, other):
        "Comparison operator"
        return ccmp(self,other) or cmp(self.name,other.name)
    def __str__(self): return Util.ccolonName(self.name)

class Declared(NamedType):
    """Class for declared types"""

    def __init__(self, language, name, declaration):
        super(Declared, self).__init__(language, name)
        self.declaration = declaration

    def accept(self, visitor): visitor.visit_declared_type(self)
    def __cmp__(self, other):
        "Comparison operator"
        return ccmp(self,other) or cmp(self.name,other.name)
    def __str__(self): return Util.ccolonName(self.name)

class Template(Declared):
    """Class for declared parametrized types"""

    def __init__(self, language, name, declaration, parameters):

        super(Template, self).__init__(language, name, declaration)
        self.parameters = parameters

    def accept(self, visitor): visitor.visit_template(self)
    def __cmp__(self, other):
        "Comparison operator"
        return ccmp(self,other) or cmp(self.parameters,other.parameters)
    def __str__(self):
        return "template<%s>%s"%(','.join(str(self.parameters)),
                                 Util.ccolonName(self.name))

class ModifierType(Type):
    """Class for alias types with modifiers (such as 'const', '&', etc.)"""

    def __init__(self, language, alias, premod, postmod):
        super(ModifierType, self).__init__(language)
        self.alias = alias
        self.premod = premod
        self.postmod = postmod

    def accept(self, visitor): visitor.visit_modifier_type(self)

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
        super(Array, self).__init__(language)
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

        super(Parametrized, self).__init__(language)
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

        super(Function, self).__init__(language)
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
                if isinstance(self[i], UnknownType):
                    self[i] = dict[i]
                else:
                    pass
            else: self[i] = dict[i]


class Declaration(object):
    """Declaration base class. Every declaration has a name, type,
    accessibility and annotations. The default accessibility is DEFAULT except for
    C++ where the Parser always sets it to one of the other three. """
    
    #__metaclass__ = Debugger

    def __init__(self, file, line, type, name):

        self.file  = file
        self.line  = line
        self.name = name
        self.type = type
        self.accessibility = DEFAULT
        self.annotations = {}

    def _set_name(self, name): self._name = tuple(name)
    name = property(lambda self: self._name, _set_name)

    def accept(self, visitor):
        """Visit the given visitor"""
        visitor.visit_declaration(self)


class Builtin(Declaration):
   """A node for internal use only."""

   def accept(self, visitor): visitor.visit_builtin(self)


class UsingDirective(Builtin):
   """Import one module's content into another."""

   def accept(self, visitor): visitor.visit_using_directive(self)


class UsingDeclaration(Builtin):
    """Import a declaration into this module."""

    def __init__(self, file, line, type, name, alias):

        super(UsingDeclaration, self).__init__(file, line, type, name)
        self.alias = alias

    def accept(self, visitor):
        visitor.visit_using_declaration(self)


class Macro(Declaration):
   """A preprocessor macro. Note that macros are not strictly part of the
   ASG, and as such are always in the global scope. A macro is "temporary" if
   it was #undefined in the same file it was #defined in."""

   def __init__(self, file, line, type, name, parameters, text):

      Declaration.__init__(self, file, line, type, name)
      self.parameters = parameters
      self.text = text

   def accept(self, visitor): visitor.visit_macro(self)


class Forward(Declaration):
   """Forward declaration"""

   def __init__(self, file, line, type, name):

      Declaration.__init__(self, file, line, type, name)
      self.template = None
      self.primary_template = None
      self.specializations = []

   def accept(self, visitor):

       visitor.visit_forward(self)


class Group(Declaration):
   """Base class for groups which contain declarations.
   This class doesn't correspond to any language construct.
   Rather, it may be used with comment-embedded grouping tags
   to regroup declarations that are to appear together in the
   manual."""

   def __init__(self, file, line, type, name):

      Declaration.__init__(self, file, line, type, name)
      self.declarations = []

   def accept(self, visitor):

      visitor.visit_group(self)


class Scope(Declaration):
   """Base class for scopes (named groups)."""

   def __init__(self, file, line, type, name):

      Declaration.__init__(self, file, line, type, name)
      self.declarations = []

   def accept(self, visitor): visitor.visit_scope(self)


class Module(Scope):
   """Module class"""
   def __init__(self, file, line, type, name):

      Scope.__init__(self, file, line, type, name)

   def accept(self, visitor): visitor.visit_module(self)


class MetaModule(Module):
   """Module Class that references all places where this Module occurs"""

   def __init__(self, type, name):

      Scope.__init__(self, None, "", type, name)
      self.module_declarations = []

   def accept(self, visitor): visitor.visit_meta_module(self)


class Inheritance(object):
   """Inheritance class. This class encapsulates the information about an
   inheritance, such as attributes like 'virtual' and 'public' """

   def __init__(self, type, parent, attributes):
      self.type = type
      self.parent = parent
      self.attributes = attributes

   def accept(self, visitor): visitor.visit_inheritance(self)
   

class Class(Scope):

   def __init__(self, file, line, type, name):

      Scope.__init__(self, file, line, type, name)
      self.parents = []
      self.primary_template = None

   def accept(self, visitor): visitor.visit_class(self)


class ClassTemplate(Scope):

   def __init__(self, file, line, type, name, template = None):

      Scope.__init__(self, file, line, type, name)
      self.parents = []
      self.template = template
      self.primary_template = None
      self.specializations = []

   def accept(self, visitor): visitor.visit_class_template(self)


class Typedef(Declaration):

   def __init__(self, file, line, type, name, alias, constr):
      Declaration.__init__(self, file, line, type, name)
      self.alias = alias
      self.constr = constr

   def accept(self, visitor): visitor.visit_typedef(self)


class Enumerator(Declaration):
   """Enumerator of an Enum. Enumerators represent the individual names and
   values in an enum."""
   
   def __init__(self, file, line, name, value):
      Declaration.__init__(self, file, line, "enumerator", name)
      self.value = value

   def accept(self, visitor): visitor.visit_enumerator(self)

class Enum(Declaration):
   """Enum declaration. The actual names and values are encapsulated by
   Enumerator objects."""

   def __init__(self, file, line, name, enumerators):

      Declaration.__init__(self, file, line, "enum", name)
      self.enumerators = enumerators[:]
      #FIXME: the Cxx parser will append a Builtin('eos') to the
      #list of enumerators which we need to extract here.
      self.eos = None
      if self.enumerators and isinstance(self.enumerators[-1], Builtin):
         self.eos = self.enumerators.pop()

   def accept(self, visitor): visitor.visit_enum(self)


class Variable(Declaration):
   """Variable definition"""

   def __init__(self, file, line, type, name, vtype, constr):
      Declaration.__init__(self, file, line, type, name)
      self.vtype  = vtype
      self.constr  = constr

   def accept(self, visitor): visitor.visit_variable(self)
    

class Const(Declaration):
   """Constant declaration. A constant is a name with a type and value."""

   def __init__(self, file, line, type, ctype, name, value):
      Declaration.__init__(self, file, line, type, name)
      self.ctype  = ctype
      self.value = value

   def accept(self, visitor): visitor.visit_const(self)
       

class Parameter(object):
   """Function Parameter"""
   
   def __init__(self, premod, type, postmod, name='', value=''):
      self.premodifier = premod
      self.type = type
      self.postmodifier = postmod
      self.name = name
      self.value = value

   def accept(self, visitor): visitor.visit_parameter(self)
   
   def __cmp__(self, other):
      "Comparison operator"
      #print "Parameter.__cmp__"
      return cmp(self.type,other.type)
   def __str__(self):
      return "%s%s%s"%(' '.join(self.premodifier),
                       str(self.type),
                       ' '.join(self.postmodifier))

class Function(Declaration):
   """Function declaration.
   Note that function names are stored in mangled form to allow overriding.
   Formatters should use the real_name to extract the unmangled name."""

   def __init__(self, file, line, type, premod, return_type, postmod, name, real_name):
      Declaration.__init__(self, file, line, type, name)
      self._real_name = real_name
      self.premodifier = premod
      self.return_type = return_type
      self.parameters = []
      self.postmodifier = postmod
      self.exceptions = []

   real_name = property(lambda self: self.name[:-1] + (self._real_name,))

   def accept(self, visitor): visitor.visit_function(self)

   def __cmp__(self, other):
      "Recursively compares the typespec of the function"
      return ccmp(self,other) or cmp(self.parameters, other.parameters)


class FunctionTemplate(Function):

   def __init__(self, file, line, type, premod, return_type, postmod, name, real_name, template = None):
      Function.__init__(self, file, line, type, premod, return_type, postmod, name, real_name)
      self.template = template

   def accept(self, visitor): visitor.visit_function_template(self)


class Operation(Function):
   """Operation class. An operation is related to a Function and is currently
   identical.
   """
   def __init__(self, file, line, type, premod, return_type, postmod, name, real_name):
      Function.__init__(self, file, line, type, premod, return_type, postmod, name, real_name)

   def accept(self, visitor): visitor.visit_operation(self)


class OperationTemplate(Operation):

   def __init__(self, file, line, type, premod, return_type, postmod, name, real_name, template = None):
      Operation.__init__(self, file, line, type, premod, return_type, postmod, name, real_name)
      self.template = template

   def accept(self, visitor): visitor.visit_operation_template(self)


class Visitor(object):
   """Visitor for ASG nodes"""

   def visit_base_type(self, type): pass
   def visit_unknown_type(self, type): pass
   def visit_declared_type(self, type): pass
   def visit_modifier_type(self, type): pass
   def visit_array(self, type): pass
   def visit_template(self, type): pass
   def visit_parametrized(self, type): pass
   def visit_function_type(self, type): pass
   def visit_dependent(self, type): pass

   def visit_declaration(self, node): pass
   def visit_builtin(self, node):
      """Visit a Builtin instance. By default do nothing. Processors who
      operate on Builtin nodes have to provide an appropriate implementation."""
      pass
   def visit_using_directive(self, node): self.visit_builtin(node)
   def visit_using_declaration(self, node): self.visit_builtin(node)
   def visit_macro(self, node): self.visit_declaration(node)
   def visit_forward(self, node): self.visit_declaration(node)
   def visit_group(self, node):
      self.visit_declaration(node)
      for d in node.declarations: d.accept(self)
   def visit_scope(self, node): self.visit_group(node)
   def visit_module(self, node): self.visit_scope(node)
   def visit_meta_module(self, node): self.visit_module(node)
   def visit_class(self, node): self.visit_scope(node)
   def visit_class_template(self, node): self.visit_class(node)
   def visit_typedef(self, node): self.visit_declaration(node)
   def visit_enumerator(self, node): self.visit_declaration(node)
   def visit_enum(self, node):
      self.visit_declaration(node)
      for e in node.enumerators:
         e.accept(self)
      if node.eos:
         node.eos.accept(self)
   def visit_variable(self, node): self.visit_declaration(node)
   def visit_const(self, node): self.visit_declaration(node)
   def visit_function(self, node):
      self.visit_declaration(node)
      for parameter in node.parameters: parameter.accept(self)
   def visit_function_template(self, node): self.visit_function(node)
   def visit_operation(self, node): self.visit_function(node)
   def visit_operation_template(self, node): self.visit_operation(node)
   def visit_parameter(self, node): pass
   def visit_inheritance(self, node): pass