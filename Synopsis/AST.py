#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Abstract Syntax Tree classes.

This file contains classes which encapsulate nodes in the AST. The base class
is the Declaration class that encapsulates a named declaration. All names used
are scoped tuples.

Also defined in module scope are the constants DEFAULT, PUBLIC, PROTECTED and
PRIVATE.
"""

import Util, Type
import string, sys, cPickle, types, stat

# The version of the file format - this should be increased everytime
# incompatible changes are made to the AST or Type classes
FILE_VERSION = 5

# Accessibility constants
DEFAULT = 0
PUBLIC = 1
PROTECTED = 2
PRIVATE = 3

def ccmp(a,b):
   """Compares classes of two objects"""
   return cmp(type(a),type(b)) or cmp(a.__class__,b.__class__)

def load(filename):
   """Loads an AST object from the given filename"""
   try:
      file = open(filename, 'rb')
      unpickler = cPickle.Unpickler(file)
      version = unpickler.load()
      if version is not FILE_VERSION:
         file.close()
         raise Exception, 'Wrong file version'
      deps = unpickler.load() # ignored here
      ast = unpickler.load()
      file.close()
      return ast
   except:
      exc, msg = sys.exc_info()[0:2]
      if exc is Exception:
         raise Exception, "Loading AST from '%s': %s"%(filename, msg)
      raise Exception, "Loading AST from '%s', %s: %s"%(filename, exc, msg)

def load_deps(filename):
   """Loads a dependencies object from the given filename. The file will be
   an AST save file (usually *.syn), but this method only reads up to the
   dependencies object stored before the actual AST. The object returned is a
   list of (filename, timestamp) pairs."""
   try:
      file = open(filename, 'rb')
      unpickler = cPickle.Unpickler(file)
      version = unpickler.load()
      if version is not FILE_VERSION:
         file.close()
         raise Exception, 'Wrong file version'
      deps = unpickler.load()
      # stop here, before loading the (probably huge) AST object
      file.close()
      return deps
   except:
      exc, msg = sys.exc_info()[0:2]
      if exc is Exception:
         raise Exception, "Loading dependencies from '%s': %s"%(filename, msg)
      raise Exception, "Loading dependencies from '%s', %s: %s"%(filename, exc, msg)

def save(filename, ast):
   """Saves an AST object to the given filename"""
   try:
      deps = make_deps(ast)
      file = open(filename, 'wb')
      pickler = cPickle.Pickler(file, 1)
      pickler.dump(FILE_VERSION)
      pickler.dump(deps)
      pickler.dump(ast)
      file.close()
   except:
      exc, msg = sys.exc_info()[0:2]
      raise Exception, "Saving '%s', %s: %s"%(filename, exc, msg)

def make_deps(ast):
   """Creates the dependencies object to save in the .syn file from the given
   AST. The dependencies are a list of (filename, timestamp) pairs, extracted
   from ast.files()"""
   deps = []
   for file in ast.files().values():
      filename = file.full_filename()
      try:
         info = os.stat(filename)
      except:
         # Ignore any file we can't stat
         continue
      time = info[stat.ST_MTIME]
      deps.append( (filename, time) )
   return deps

class AST:
   """Top-level Abstract Syntax Tree.
   The AST represents the whole AST for a file or group of files as a list of
   declarations and a types dictionary.
   """

   def __init__(self, files=None, declarations=None, typedict=None):
      """Constructor"""

      if files is None: files = {}
      if declarations is None: declarations = []
      if type(files) is not types.DictType: raise TypeError, "files parameter must be a dict of SourceFile objects"
      if type(declarations) != type([]): raise TypeError, "declarations parameter must be a list of declarations"
      if typedict is None: typedict = Type.Dictionary()
      elif not isinstance(typedict, Type.Dictionary): raise TypeError, "types must be an instance of Type.Dictionary"
      self.__files	    = files
      self.__declarations = list(declarations)
      self.__types	    = typedict

   def files(self):
      """The files this AST represents. Returns a dictionary mapping
      filename to SourceFile objects."""	
      return self.__files

   def declarations(self):
      """List of Declaration objects. These are the declarations at file scope"""
      return self.__declarations

   def types(self):
      """Dictionary of types. This is a Type.Dictionary object"""
      return self.__types

   def accept(self, visitor):
      """Accept the given visitor"""
      visitor.visitAST(self)

   def merge(self, other_ast):
      """Merges another AST. Files and declarations are appended to those in
      this AST, and types are merged by overwriting existing types -
      Unduplicator is responsible for sorting out the mess this may cause :)"""
      self.__types.merge(other_ast.types())
      self.__declarations.extend(other_ast.declarations())
      #merge files
      replacement = {}
      for filename, file in other_ast.files().items():
         if not self.__files.has_key(filename):
            self.__files[filename] = file
            continue
         myfile = self.__files[filename]
         replacement[file] = myfile
         # the 'main' flag dominates...
         myfile.set_is_main(myfile.is_main() or file.is_main())
         myfile.declarations().extend(file.declarations())
         myfile.includes().extend(file.includes())
      # fix dangling inclusions of 'old' files
      for r in replacement:
         for f in self.__files.values():
            for i in f.includes():
               if i.target() == r: i.set_target(replacement[r])

class Include:
   """Information about an include directive in a SourceFile.
   If the include directive required a macro expansion to get the filename,
   the is_macro will return true. If the include directive was actually an
   include_next, then is_next will return true.
   """
   def __init__(self, target, name, is_macro, is_next):
      if not isinstance(target, SourceFile):
         raise TypeError, "target parameter must be a SourceFile"
      self.__target = target
      self.__name = name
      self.__is_macro = is_macro
      self.__is_next = is_next

   def target(self):
      return self.__target

   def set_target(self, target):
      self.__target = target

   def name(self):
      """return the name as it appears in the include statement"""
      return self.__name

   def is_macro(self):
      return self.__is_macro

   def is_next(self):
      return self.__is_next

class MacroCall:
   """A class to support mapping from positions in a preprocessed file
   back to positions in the original file."""

   def __init__(self, name, start, end, diff):

      self.name = name
      self.start = start
      self.end = end
      self.diff = diff

class SourceFile:
   """The information about a file that the AST was generated from.
   Contains filename, all declarations from this file (even nested ones) and
   includes (aka imports) from this file."""

   def __init__(self, filename, full_filename, language):
      """Constructor"""
      if type(filename) is not types.StringType: raise TypeError, "filename parameter must be a string filename"
      if type(full_filename) is not types.StringType: raise TypeError, "full_filename parameter must be a string filename"
      if type(language) is not types.StringType: raise TypeError, "language parameter must be a string language"
      self.__filename = filename
      self.__full_filename = full_filename
      self.__language = language
      self.__includes = []
      self.__declarations = []
      self.__is_main = 0
      self.__macro_calls = {}

   def is_main(self):
      """Returns whether this was a main file. A source file is a main file
      if it was parsed directly or as an extra file. A source file is not a
      main file if it was just included. A source file that had no actual
      declarations but was given to the parser as either the main source
      file or an extra file is still a main file."""
      return self.__is_main
   
   def set_is_main(self, value):
      """Sets the is_main flag. Typically only called once, and then may by
      the linker if it discovers that a file is actually a main file
      elsewhere."""
      self.__is_main = value

   def filename(self):
      """Returns the filename of this file. The filename can be absolute or
      relative, depending on the settings for the Parser"""
      return self.__filename

   def full_filename(self):
      """Returns the full_filename of this file. The filename can be absolute or
      relative, depending on the filename given to the Parser. This filename
      does not have any basename stripped from it, and should be accessible
      from the current directory as is whether absolute or relative."""
      return self.__full_filename

   def language(self):
      """Returns the language for this file as a string"""
      return self.__language

   def declarations(self):
      """Returns a list of declarations declared in this file"""
      return self.__declarations

   def includes(self):
      """Returns a the files included by this file. These may be
      absolute or relative, depending on the settings for the Parser. This
      may also include system files. The return value is a list of tuples,
      with each tuple being a pair (included-from-filename,
      include-filename). This allows distinction of files directly included
      (included-from-filename == self.filename()) but also allows dependency
      tracking since *all* files read while parsing this file are included
      in the list (even system files)."""
      return self.__includes

   def macro_calls(self):
      return self.__macro_calls
    
class Declaration:
   """Declaration base class. Every declaration has a name, type,
   accessibility and annotations. The default accessibility is DEFAULT except for
   C++ where the Parser always sets it to one of the other three. """
    
   def __init__(self, file, line, language, strtype, name):
      if file is not None:
         if type(file) is not types.InstanceType or not isinstance(file, SourceFile):
            raise TypeError, "file must be a SourceFile object"
      self.__file  = file
      self.__line  = line
      self.__language = language
      self.__name = tuple(name)
      self.__type = strtype
      self.__accessibility = DEFAULT
      self.annotations = {}

   def file(self):
      """The SourceFile this declaration appeared in"""
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
   def accept(self, visitor):
      """Visit the given visitor"""
      visitor.visitDeclaration(self)
   def accessibility(self):
      """One of the accessibility constants.
      This may be one of DEFAULT, PUBLIC, PROTECTED or PRIVATE, which are
      defined at module scope (Synopsis.AST)"""
      return self.__accessibility

   def set_name(self, name):
      """Change the name of the declaration. If you do want to change the
      name (and you probably don't!) then make sure you update your 'types'
      dictionary too!"""
      self.__name = tuple(name)
   def set_accessibility(self, axs):
      """Change the accessibility"""
      self.__accessibility = axs

class Builtin (Declaration):
   """An ast node for internal use only."""

   def __init__(self, file, line, language, type, name):
      """Constructor"""

      Declaration.__init__(self, file, line, language, type, name)

   def accept(self, visitor): visitor.visitBuiltin(self)

class Macro (Declaration):
   """A preprocessor macro. Note that macros are not strictly part of the
   AST, and as such are always in the global scope. A macro is "temporary" if
   it was #undefined in the same file it was #defined in."""

   def __init__(self, file, line, language, type, name, parameters, text):
      """Constructor"""
      Declaration.__init__(self, file, line, language, type, name)
      self.__parameters = parameters
      self.__text = text

   def parameters(self):
      """Returns a list of parameter names (strings) for this macro if it
      has any. Note that if the macro is not a function-like macro, this
      method will return None. If it is a function-like macro but with no
      parameters, an empty list will be returned."""
      return self.__parameters

   def text(self):
      """Returns the replacement text for this macro as a string"""
      return self.__text

   def accept(self, visitor): visitor.visitMacro(self)

class Forward (Declaration):
   """Forward declaration"""

   def __init__(self, file, line, language, type, name):
      Declaration.__init__(self, file, line, language, type, name)
   def accept(self, visitor): visitor.visitForward(self)

class Group (Declaration):
   """Base class for groups which contain declarations.
   This class doesn't correspond to any language construct.
   Rather, it may be used with comment-embedded grouping tags
   to regroup declarations that are to appear together in the
   manual."""

   def __init__(self, file, line, language, type, name):
      Declaration.__init__(self, file, line, language, type, name)
      self.__declarations = []
   def declarations(self):
      """The list of declarations in this group"""
      return self.__declarations
   def accept(self, visitor):
      #print "group accept", visitor
      visitor.visitGroup(self)

class Scope (Group):
   """Base class for scopes (named groups)."""

   def __init__(self, file, line, language, type, name):
      Group.__init__(self, file, line, language, type, name)

   def accept(self, visitor): visitor.visitScope(self)

class Module (Scope):
   """Module class"""
   def __init__(self, file, line, language, type, name):
      Scope.__init__(self, file, line, language, type, name)

   def accept(self, visitor): visitor.visitModule(self)

class MetaModule (Module):
   """Module Class that references all places where this Module occurs"""
   def __init__(self, lang, type, name):
      Scope.__init__(self, None, "", lang, type, name)
      self.__module_declarations = []
   def module_declarations(self):
      """The module declarations this metamodule subsumes"""
      return self.__module_declarations
   def accept(self, visitor): visitor.visitMetaModule(self)

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
   constr()          -- boolean: true if the alias type was constructed within this typedef declaration."""
   def __init__(self, file, line, language, type, name, alias, constr):
      Declaration.__init__(self, file, line, language, type, name)
      self.__alias = alias
      self.__constr = constr
   def alias(self):
      """The Type object aliased by this typedef"""
      return self.__alias
   def constr(self):
      """True if alias type was constructed here.
      For example, typedef struct _Foo {} Foo;"""
      return self.__constr
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
      self.__enumerators = enumerators[:]
      #FIXME: the Cxx parser will append a Builtin('eos') to the
      #list of enumerators which we need to extract here.
      self.eos = None
      if self.__enumerators and isinstance(self.__enumerators[-1], Builtin):
         self.eos = self.__enumerators.pop()

   def enumerators(self):
      """List of Enumerator objects"""
      return self.__enumerators
   def accept(self, visitor): visitor.visitEnum(self)

class Variable (Declaration):
   """Variable definition"""

   def __init__(self, file, line, language, type, name, vtype, constr):
      Declaration.__init__(self, file, line, language, type, name)
      self.__vtype  = vtype
      self.__constr  = constr

   def vtype(self):
      """The Type object for this variable"""
      return self.__vtype
   def constr(self):
      """True if the type was constructed here.
      For example: struct Foo {} myFoo;"""
      return self.__constr
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

   def __init__(self, file, line, language, type, premod, returnType, postmod, name, realname):
      Declaration.__init__(self, file, line, language, type, name)
      self.__realname = realname
      self.__premodifier = premod
      self.__returnType = returnType
      self.__parameters = []
      self.__postmodifier = postmod
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
      name = list(self.name())
      name[-1] = self.__realname
      return tuple(name)
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

   def set_returnType(self, type): self.__returnType = type

   def __cmp__(self, other):
      "Recursively compares the typespec of the function"
      return ccmp(self,other) or cmp(self.parameters(), other.parameters())

class Operation (Function):
   """Operation class. An operation is related to a Function and is currently
   identical.
   """
   def __init__(self, file, line, language, type, premod, returnType, postmod, name, realname):
      Function.__init__(self, file, line, language, type, premod, returnType, postmod, name, realname)
   def accept(self, visitor): visitor.visitOperation(self)

class Visitor :
   """Visitor for AST nodes"""
   def visitAST(self, node):
      for declaration in node.declarations(): declaration.accept(self)
   def visitDeclaration(self, node): return
   def visitBuiltin(self, node): self.visitDeclaration(node)
   def visitMacro(self, node): self.visitDeclaration(node)
   def visitForward(self, node): self.visitDeclaration(node)
   def visitGroup(self, node):
      self.visitDeclaration(node)
      for declaration in node.declarations(): declaration.accept(self)
   def visitScope(self, node): self.visitGroup(node)
   def visitModule(self, node): self.visitScope(node)
   def visitMetaModule(self, node): self.visitModule(node)
   def visitClass(self, node): self.visitScope(node)
   def visitTypedef(self, node): self.visitDeclaration(node)
   def visitEnumerator(self, node): self.visitDeclaration(node)
   def visitEnum(self, node):
      self.visitDeclaration(node)
      for enum in node.enumerators(): enum.accept(self)
      if node.eos: node.eos.accept(self)
   def visitVariable(self, node): self.visitDeclaration(node)
   def visitConst(self, node): self.visitDeclaration(node)
   def visitFunction(self, node):
      self.visitDeclaration(node)
      for parameter in node.parameters(): parameter.accept(self)
   def visitOperation(self, node): self.visitFunction(node)
   def visitParameter(self, node): return
   def visitInheritance(self, node): return
