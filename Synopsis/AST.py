# $Id: AST.py,v 1.5 2001/01/22 17:06:15 stefan Exp $
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
# $Log: AST.py,v $
# Revision 1.5  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#

"""Abstract Syntax Tree classes.

This file contains classes which encapsulate nodes in the AST. The base class
is the Declaration class that encapsulates a named declaration. All names used
are scoped tuples.

Also defined in module scope are the constants DEFAULT, PUBLIC, PROTECTED and
PRIVATE.
"""
# TODO:
# Change AST.file to be AST.files() - a list of files

import string
import Util

# Accessability constants
DEFAULT = 0
PUBLIC = 1
PROTECTED = 2
PRIVATE = 3

def ccmp(a,b):
    """Compares classes of two objects"""
    return cmp(type(a),type(b)) or cmp(a.__class__,b.__class__)

class AST:
    """Top-level Abstract Syntax Tree.
    The AST represents the whole AST for a file as a list of declarations, or
    for a set of linked files."""

    def __init__(self, file, language, declarations):
	"""Constructor"""
        self.__file         = file
        self.__language     = language
        self.__declarations = declarations
    def file(self):
	"""The file name of the main file"""	
	return self.__file
    def language(self):
	"""The language for this AST"""
	return self.__language
    def declarations(self):
	"""List of Declaration objects. These are the declarations at file scope"""
	return self.__declarations
    def accept(self, visitor):
	"""Accept the given visitor"""
	visitor.visitAST(self)

class Declaration:
    """Declaration base class. Every declaration has a name, comments,
    accessability and type. The default accessability is DEFAULT except for
    C++ where the Parser always sets it to one of the other three. """

    def __init__(self, file, line, language, type, name):
        self.__file  = file
        self.__line  = line
        self.__language = language
	self.__name = tuple(name)
        self.__type = type
        self.__comments = []
	self.__accessability = DEFAULT
    def file(self):
	"""The file this declaration appeared in"""
	return self.__file
    def line(self):
	"""The line of the file this declaration started at"""
	return self.__line
    def language(self):
	"""The language this declaration is in"""
	return self.__language
    def type(self):
	"""A string name of the type of this declaration"""
	return self.__type
    def name(self):
	"""The scoped tuple name of this declaration"""
	return self.__name
    def comments(self):
	"""A list of Comment objects"""
	return self.__comments
    def accept(self, visitor):
	"""Visit the given visitor. For Declaration it does nothing."""
	pass
    def accessability(self):
	"""One of the accessability constants.
	This may be one of DEFAULT, PUBLIC, PROTECTED or PRIVATE, which are
	defined at module scope (Synopsis.AST)"""
	return self.__accessability
    def set_accessability(self, axs):
	"""Change the accessability"""
	self.__accessability = axs

class Forward (Declaration):
    """Forward declaration"""

    def __init__(self, file, line, language, type, name):
        Declaration.__init__(self, file, line, language, type, name)
    def accept(self, visitor): visitor.visitForward(self)

class Declarator (Declaration):
    """Declarator. This is a class, soon to be phased out, that holds
    information about each declaration for variables and typedefs. Currently
    there is only one declarator per variable or typedef declaration (they
    used to be many-to-one) which is why they will disappear."""


    def __init__(self, file, line, language, name, sizes):
        Declaration.__init__(self, file, line, language, "", name)
        self.__sizes = sizes
    def _setAlias(self, alias):
	"""Change the alias type. This is used only by the Linker"""
	self.__alias = alias

    def sizes(self):
	"""List of array sizes, or None"""
	return self.__sizes
    def alias(self):
	"""Type object if this is a typedef declarator, or None"""
	return self.__alias
    def accept(self, visitor):
	"""Accept the given visitor"""
	visitor.visitDeclarator(self)

class Scope (Declaration):
    """Base class for scopes with contained declarations."""

    def __init__(self, file, line, language, type, name):
        Declaration.__init__(self, file, line, language, type, name)
        self.__declarations = []
    def declarations(self):
	"""The list of declarations in this scope"""
	return self.__declarations

class Module (Scope):
    """Module class.
    """
    def __init__(self, file, line, language, type, name):
        Scope.__init__(self, file, line, language, type, name)
    def accept(self, visitor): visitor.visitModule(self)

class MetaModule (Module):
    """Module Class that references all places where this Module occurs"""
    def __init__(self, lang, type, name):
        Scope.__init__(self, "", "", lang, type, name)
	self.__module_declarations = []
    def accept(self, visitor): visitor.visitMetaModule(self)
    def module_declarations(self): return self.__module_declarations


class Inheritance:
    """Inheritance class. This class encapsulates the information about an
    inheritance, such as attributes like 'virtual' and 'public' """
    def __init__(self, type, parent, attributes):
        self.__type = type
        self.__parent = parent
        self.__attributes = attributes
    def type(self):
	"""The type of inheritance ('implements', 'extends' etc)"""
	return self.__type
    def parent(self):
	"""The parent class or typedef declaration"""
	return self.__parent
    def attributes(self):
	"""Attributes such as 'virtual', 'public' etc"""
	return self.__attributes
    def accept(self, visitor): visitor.visitInheritance(self)

    def set_parent(self, parent): self.__parent = parent


class Class (Scope):
    """Class class."""

    def __init__(self, file, line, language, type, name):
        Scope.__init__(self, file, line, language, type, name)
        self.__parents = []
	self.__template = None
    def parents(self):
	"""The list of Inheritance objects representing base classes"""
	return self.__parents
    def template(self):
	"""The Template Type if this is a template, or None"""
	return self.__template
    def set_template(self, template):
	self.__template = template
    def accept(self, visitor): visitor.visitClass(self)

class Typedef (Declaration):
    """Typedef class.

  alias()           -- the type object referenced by this alias
  constr()          -- boolean: true if the alias type was constructed within this typedef declaration.
  declarator()      -- the new type being declared"""
    def __init__(self, file, line, language, type, name, alias, constr, declarator):
        Declaration.__init__(self, file, line, language, type, name)
        self.__alias = alias
        self.__constr = constr
        self.__declarator = declarator
    def alias(self):
	"""The Type object aliased by this typedef"""
	return self.__alias
    def constr(self):
	"""True if alias type was constructed here.
	For example, typedef struct _Foo {} Foo;"""
	return self.__constr
    def declarator(self):
	return self.__declarator
    def accept(self, visitor): visitor.visitTypedef(self)

    def set_alias(self, type): self.__alias = type

class Enumerator (Declaration):
    """Enumerator of an Enum. Enumerators represent the individual names and
    values in an enum."""

    def __init__(self, file, line, language, name, value):
        Declaration.__init__(self, file, line, language, "enumerator", name)
        self.__value = value
    def value(self):
	"""The string value of this enumerator"""
	return self.__value
    def accept(self, visitor): visitor.visitEnumerator(self)

class Enum (Declaration):
    """Enum declaration. The actual names and values are encapsulated by
    Enumerator objects."""

    def __init__(self, file, line, language, name, enumerators):
        Declaration.__init__(self, file, line, language, "enum", name)
        self.__enumerators = enumerators
    def enumerators(self):
	"""List of Enumerator objects"""
	return self.__enumerators
    def accept(self, visitor): visitor.visitEnum(self)

class Variable (Declaration):
    """Variable definition"""

    def __init__(self, file, line, language, type, name, vtype, constr, declarator):
        Declaration.__init__(self, file, line, language, type, name)
        self.__vtype  = vtype
        self.__constr  = constr
        self.__declarator = declarator

    def vtype(self):
	"""The Type object for this variable"""
	return self.__vtype
    def constr(self):
	"""True if the type was constructed here.
	For example: struct Foo {} myFoo;"""
	return self.__constr
    def declarator(self): return self.__declarator
    def accept(self, visitor): visitor.visitVariable(self)

    def set_vtype(self, vtype): self.__vtype = vtype
    

class Const (Declaration):
    """Constant declaration. A constant is a name with a type and value."""

    def __init__(self, file, line, language, type, ctype, name, value):
        Declaration.__init__(self, file, line, language, type, name)
        self.__ctype  = ctype
        self.__value = value

    def ctype(self):
	"""Type object for this const"""
	return self.__ctype
    def value(self):
	"""The string value of this type"""
	return self.__value
    def accept(self, visitor): visitor.visitConst(self)

    def set_ctype(self, ctype): self.__ctype = ctype
    

class Parameter:
    """Function Parameter"""

    def __init__(self, premod, type, postmod, identifier='', value=''):
        self.__premodifier = premod
        self.__type = type
        self.__postmodifier = postmod
        self.__identifier = identifier
        self.__value = value
    def premodifier(self):
	"""List of premodifiers such as 'in' or 'out'"""
	return self.__premodifier
    def type(self):
	"""The Type object"""
	return self.__type
    def postmodifier(self):
	"""Post modifiers..."""
	return self.__postmodifier
    def identifier(self):
	"""The string name of this parameter"""
	return self.__identifier
    def value(self):
	"""The string value of this parameter"""
	return self.__value
    def accept(self, visitor): visitor.visitParameter(self)

    def set_type(self, type): self.__type = type

    def __cmp__(self, other):
	"Comparison operator"
	#print "Parameter.__cmp__"
	return cmp(self.type(),other.type())
    def __str__(self):
	return "%s%s%s"%(self.__premodifier,str(self.__type),self.__postmodifier)

class Function (Declaration):
    """Function declaration.
    Note that function names are stored in mangled form to allow overriding.
    Formatters should use the realname() method to extract the unmangled name."""

    def __init__(self, file, line, language, type, premod, returnType, name, realname):
        Declaration.__init__(self, file, line, language, type, name)
	self.__realname = realname
        self.__premodifier = premod
        self.__returnType = returnType
        self.__parameters = []
        self.__postmodifier = []
        self.__exceptions = []
	self.__template = None
    def premodifier(self):
	"""List of premodifiers such as 'oneway'"""
	return self.__premodifier
    def returnType(self):
	"""Type object for the return type of this function"""
	return self.__returnType
    def realname(self):
	"""The unmangled scoped name tuple of this function"""
	return self.__realname
    def parameters(self):
	"""The list of Parameter objects of this function"""
	return self.__parameters
    def postmodifier(self):
	"""The list of postmodifier strings"""
	return self.__postmodifier
    def exceptions(self):
	"""The list of exception Types"""
	return self.__exceptions
    def template(self):
	"""The Template Type if this is a template, or None"""
	return self.__template
    def set_template(self, template):
	self.__template = template
    def accept(self, visitor): visitor.visitFunction(self)

    def set_returnType(self, type): self.__type = type

    def __cmp__(self, other):
	"Recursively compares the typespec of the function"
	return ccmp(self,other) or cmp(self.parameters(), other.parameters())

class Operation (Function):
    """Operation class. An operation is related to a Function and is currently
    identical.
    """
    def __init__(self, file, line, language, type, premod, returnType, name, realname):
        Function.__init__(self, file, line, language, type, premod, returnType, name, realname)
    def accept(self, visitor): visitor.visitOperation(self)

class Comment :
    """Class containing information about a comment. There may be one or more
    lines in the text of the comment, and language-specific comment characters
    are not stripped."""

    def __init__(self, text, file, line):
        self.__text = text
        self.__file = file
        self.__line = line

    def text(self):
	"""The text of the comment"""
	return self.__text
    def __str__(self):
	"""Returns the text of the comment"""
	return self.__text
    def file(self):
	"""The file it was defined in"""
	return self.__file
    def line(self):
	"""The line it was defined at"""
	return self.__line

class Visitor :
    """Visitor for AST nodes"""
    def visitAST(self, node): return
    def visitDeclaration(self, node): return
    def visitForward(self, node): self.visitDeclaration(node)
    def visitDeclarator(self, node): self.visitDeclaration(node)
    def visitScope(self, node): self.visitDeclaration(node)
    def visitModule(self, node): self.visitScope(node)
    def visitMetaModule(self, node): self.visitModule(node)
    def visitClass(self, node): self.visitScope(node)
    def visitTypedef(self, node): self.visitDeclaration(node)
    def visitEnumerator(self, node): self.visitDeclaration(node)
    def visitEnum(self, node): self.visitDeclaration(node)
    def visitVariable(self, node): self.visitDeclaration(node)
    def visitConst(self, node): self.visitDeclaration(node)
    def visitFunction(self, node): self.visitDeclaration(node)
    def visitOperation(self, node): self.visitDeclaration(node)
    def visitParameter(self, node): return
    def visitComment(self, node): return
    def visitInheritance(self, node): return
