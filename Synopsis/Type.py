# $Id: Type.py,v 1.6 2001/01/24 18:06:45 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
# Copyright (C) 2000, 2001 Stephen Davies
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: Type.py,v $
# Revision 1.6  2001/01/24 18:06:45  stefan
# fixed the Unknown type to have a name *and* a link attribute, to distinguish the written label from the link it refers to
#
# Revision 1.5  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
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
    """Type abstract class.

Function:

  language()        -- the language this type was defined in
  accept(visitor)   -- visitor pattern accept. See idlvisitor.py."""

    def __init__(self, language):
        self.__language = language
    def language(self): return self.__language
    def accept(self, visitor): pass

    def __cmp__(self, other):
	"Comparison operator"
	#print "Type.__cmp__"
	return cmp(id(self),id(other))

class Base (Type):
    """Class for base types. (Type)

Functions:

  name()        -- Simple name of the type."""

    def __init__(self, language, name):
        Type.__init__(self, language)
        self.__name  = name
	if type(name) != type(()) and type(name) != type([]):
	    raise TypeError,"Name must be scoped"
    def name(self): return self.__name
    def accept(self, visitor): visitor.visitBaseType(self)
    def __cmp__(self, other):
	"Comparison operator"
	#print "Base.__cmp__"
	return ccmp(self,other) or \
	    cmp(self.name(),other.name())
    def __str__(self): return Util.ccolonName(self.__name)

class Unknown(Type):
    """Class for not (yet) known type"""
    base = Type
    def __init__(self, language, name):
        Type.__init__(self, language)
        self.__name = name
        self.__link = name
	if type(name) != type(()) and type(name) != type([]):
	    raise TypeError,"Name must be scoped"
    def name(self): return self.__name
    def link(self): return self.__link
    def resolve(self, language, name, link):
        self.base.__language = language
        self.__name = name
        self.__link = link
    def accept(self, visitor): visitor.visitUnknown(self)
    def __cmp__(self, other):
	"Comparison operator"
	#print "Unknown.__cmp__"
	return ccmp(self,other) or cmp(self.name(),other.name())
    def __str__(self): return Util.ccolonName(self.__name)

class Declared (Type):
    """Class for declared types (Type)

Functions:

  declaration() -- Decl object which corresponds to this type.
  name()        -- scoped name of the type."""

    def __init__(self, language, name, declaration):
        Type.__init__(self, language)
        self.__name  = name
        self.__declaration = declaration
    def declaration(self): return self.__declaration
    def name(self): return self.__name
    def accept(self, visitor): visitor.visitDeclared(self)
    def __cmp__(self, other):
	"Comparison operator"
	#print "Declared.__cmp__"
	return ccmp(self,other) or cmp(self.name(),other.name())
    def __str__(self): return Util.ccolonName(self.__name)

class Template (Declared):
    """Class for declared parametrized types
Functions:

  parameters()  -- list of type names used to declare this template"""

    def __init__(self, language, name, declaration, parameters):
        Declared.__init__(self, language, name, declaration)
        self.__parameters = parameters
    def parameters(self): return self.__parameters
    def accept(self, visitor): visitor.visitTemplate(self)
    def __cmp__(self, other):
	"Comparison operator"
	#print "Template.__cmp__"
	return ccmp(self,other) or cmp(self.parameters(),other.parameters())
    def __str__(self):
	return "template<%s>%s"%(
	    string.join(self.__parameters,','),
	    Util.ccolonName(self.name())
	)

class Modifier (Type):
    """Class for alias types with modifiers (such as 'const', '&', etc.)
Functions:

  alias()    -- the type this type refers to
  premod()   -- the modifier string
  postmod()  -- the modifier string"""

    def __init__(self, language, alias, premod, postmod):
        Type.__init__(self, language)
        self.__alias = alias
        self.__premod = premod
        self.__postmod = postmod
    def alias(self): return self.__alias
    def premod(self): return self.__premod
    def postmod(self): return self.__postmod
    def accept(self, visitor): visitor.visitModifier(self)

    def set_alias(self, alias): self.__alias = alias
    def __cmp__(self, other):
	"Comparison operator"
	#print "Modifier.__cmp__",self,other
	return ccmp(self,other) or \
	    cmp(self.alias(),other.alias()) or \
	    cmp(self.premod(),other.premod()) or \
	    cmp(self.postmod(),other.postmod())
    def __str__(self): 
	return "%s%s%s"%(
	    string.join(map(lambda s:s+" ",self.__premod),''),
	    str(self.__alias),
	    string.join(self.__postmod,'')
	)

class Parametrized (Type):
    """Class for parametrized type instances
Functions:

  template()    -- template type this is an instance of
  parameters()  -- list of types for which this template is instanciated"""

    def __init__(self, language, template, parameters):
        Type.__init__(self, language)
        self.__template = template
        self.__parameters = parameters
    def template(self): return self.__template
    def parameters(self): return self.__parameters
    def accept(self, visitor): visitor.visitParametrized(self)
    def set_template(self, type): self.__template = type
    def __cmp__(self, other):
	"Comparison operator"
	#print "Parametrized.__cmp__"
	return ccmp(self,other) or \
	    cmp(self.template(),other.template())
    def __str__(self):
	return "%s<%s>"%(
	    self.__template.name(),
	    string.join(map(str,self.__parameters),',')
	)

class Function (Type):
    """Class for function pointer types.
    Functions:
	returnType()	-- nested return type
	premodifiers()	-- list of premodifier strings
	parameters()	-- list of nested parameter types
    """
    def __init__(self, language, retType, premod, params):
	Type.__init__(self, language)
	self.__returnType = retType
	self.__premod = premod
	self.__params = params
    def returnType(self): return self.__returnType
    def premod(self): return self.__premod
    def parameters(self): return self.__params
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
            if self.has_key(i): pass ##print "Error: multiple types ", Util.ccolonName(i)
            else: self[i] = dict[i]

class Visitor:
    """Visitor for Type objects"""
    def visitBaseType(self, type): return
    def visitUnknown(self, type): return
    def visitDeclared(self, type): return
    def visitModifier(self, type): return
    def visitTemplate(self, type): return
    def visitParametrized(self, type): return
    def visitFunctionType(self, type): return
