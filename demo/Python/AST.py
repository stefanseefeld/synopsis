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
    """Class for top-level Abstract Syntax Tree.

Functions:

  file()          -- the file name of the main IDL file.
  declarations()  -- list of Decl objects corresponding to declarations
                     at file scope.
  accept(visitor) -- visitor pattern accept. See Visitor.py."""

    def __init__(self, file, language, declarations):
        self.__file         = file
        self.__language     = language
        self.__declarations = declarations

    def file(self):            return self.__file
    def language(self):        return self.__language
    def declarations(self):    return self.__declarations
    def accept(self, visitor): visitor.visitAST(self)

class Declaration:
    """Declaration base class

    file()            -- the file this type was defined in
    line()            -- the line this type was defined in
    mainFile()        -- boolean: true if the file was the main source file;
			 false if it was an included file.
    language()        -- the language this type was defined in
    name()            -- the scoped name of this declaration
    type()            -- a string representing the type (as used in the original declaration)
    comments()        -- list of Comment objects containing comments which
			 immediately followed this declaration.
    accept(visitor)   -- visitor pattern accept.
    accessability()   -- access specifier, DEFAULT, PUBLIC, PROTECTED or PRIVATE.
    """

    def __init__(self, file, line, main, language, type, name):
        self.__file  = file
        self.__line  = line
        self.__main  = main
        self.__language = language
	self.__name = tuple(name)
        self.__type = type
        self.__comments = []
	self.__accessability = DEFAULT
    def file(self): return self.__file
    def line(self): return self.__line
    def mainfile(self): return self.__main
    def language(self): return self.__language
    def type(self): return self.__type
    def name(self): return self.__name
    def comments(self): return self.__comments
    def accept(self, visitor): pass
    def accessability(self): return self.__accessability
    def set_accessability(self, axs): self.__accessability = axs

class Forward (Declaration):
    """Forward declaration

    Functions:

    """

    def __init__(self, file, line, main, language, type, name):
        Declaration.__init__(self, file, line, main, language, type, name)
    def accept(self, visitor): visitor.visitForward(self)

class Declarator (Declaration):
    """Declarator

Functions:

  sizes() -- list of array sizes, or None if this is a simple
             declarator.
  alias() -- Typedef object for this declarator if this is a typedef
             declarator. None otherwise."""

    def __init__(self, file, line, main, language, name, sizes):
        Declaration.__init__(self, file, line, main, language, "", name)
        self.__sizes = sizes
    def _setAlias(self, alias): self.__alias = alias

    def sizes(self): return self.__sizes
    def alias(self): return self.__alias
    def accept(self, visitor): visitor.visitDeclarator(self)

class Scope (Declaration):
    """Scope class.

  name()            -- name of the scope
  declarations()    -- the declarations inside this scope"""
    def __init__(self, file, line, main, language, type, name):
        Declaration.__init__(self, file, line, main, language, type, name)
        self.__declarations = []
    def declarations(self): return self.__declarations

class Module (Scope):
    """Module class.
    """
    def __init__(self, file, line, main, language, type, name):
        Scope.__init__(self, file, line, main, language, type, name)
    def accept(self, visitor): visitor.visitModule(self)

class MetaModule (Module):
    """Module Class that references all places where this Module occurs"""
    def __init__(self, type, name):
        Scope.__init__(self, "", "", 1, "", type, name)
	self.__module_declarations = []
    def accept(self, visitor): visitor.visitMetaModule(self)
    def module_declarations(self): return self.__module_declarations


class Inheritance:
    """Inheritance class.

  type()       -- the type of inheritance ('implements', 'extends' etc.)
  parent()     -- the parent (a Forward Type)
  attributes() -- the attributes of this inheritance (virtual, public etc."""
    def __init__(self, type, parent, attributes):
        self.__type = type
        self.__parent = parent
        self.__attributes = attributes
    def type(self): return self.__type
    def parent(self): return self.__parent
    def attributes(self): return self.__attributes
    def accept(self, visitor): visitor.visitInheritance(self)

    def set_parent(self, parent): self.__parent = parent


class Class (Scope):
    """Class class.

    parents()      -- the parents this Class is derived from
    declarations() -- the declarations declared in this class"""

    def __init__(self, file, line, main, language, type, name):
        Scope.__init__(self, file, line, main, language, type, name)
        self.__parents = []
        self.__declarations = []
    def parents(self): return self.__parents
    def declarations(self): return self.__declarations
    def accept(self, visitor): visitor.visitClass(self)

class Typedef (Declaration):
    """Typedef class.

  alias()           -- the type object referenced by this alias
  constr()          -- boolean: true if the alias type was constructed within this typedef declaration.
  declarator()      -- the new type being declared"""
    def __init__(self, file, line, main, language, type, name, alias, constr, declarator):
        Declaration.__init__(self, file, line, main, language, type, name)
        self.__alias = alias
        self.__constr = constr
        self.__declarator = declarator
    def alias(self): return self.__alias
    def constr(self): return self.__constr
    def declarator(self): return self.__declarator
    def accept(self, visitor): visitor.visitTypedef(self)

    def set_alias(self, type): self.__alias = type

class Enumerator (Declaration):
    """Enumerator of an Enum

  name() -- name of the enumeration.
  value() -- value of the enumeration."""

    def __init__(self, file, line, main, language, name, value):
        Declaration.__init__(self, file, line, main, language, "enumerator", name)
        self.__value = value
    def value(self): return self.__value
    def accept(self, visitor): visitor.visitEnumerator(self)

class Enum (Declaration):
    """Enum declaration

Function:

  enumerators() -- enumerators associated with this enum."""

    def __init__(self, file, line, main, language, name, enumerators):
        Declaration.__init__(self, file, line, main, language, "enum", name)
        self.__enumerators = enumerators
    def enumerators(self): return self.__enumerators
    def accept(self, visitor): visitor.visitEnum(self)

class Variable (Declaration):
    """Variable

Functions:

  vtype()       -- Type object for the type of this variable.
  constr()      -- boolean: true if the variable's type was constructed within the variable declaration.
  declarator()  -- Declarator object for this Variable."""

    def __init__(self, file, line, main, language, type, name, vtype, constr, declarator):
        Declaration.__init__(self, file, line, main, language, type, name)
        self.__vtype  = vtype
        self.__constr  = constr
        self.__declarator = declarator

    def vtype(self): return self.__vtype
    def constr(self): return self.__constr
    def declarator(self): return self.__declarator
    def accept(self, visitor): visitor.visitVariable(self)

    def set_vtype(self, vtype): self.__vtype = vtype
    

class Const (Declaration):
    """Const

Functions:

  ctype()  -- Type object for the type of this const.
  name()   -- name
  value()  -- value of the constant (string).
              """

    def __init__(self, file, line, main, language, type, ctype, name, value):
        Declaration.__init__(self, file, line, main, language, type, name)
        self.__ctype  = ctype
        self.__value = value

    def ctype(self): return self.__ctype
    def value(self): return self.__value
    def accept(self, visitor): visitor.visitConst(self)

    def set_ctype(self, ctype): self.__ctype = ctype
    

class Parameter:# (Declaration):
    """Parameter class.

  premodifier()  -- the premodifier ('const', 'in' etc.)
  type()         -- the type of this parameter
  postmodifier() -- the postmodifier
  identifier()   -- the name
  value()        -- the value"""
    def __init__(self, premod, type, postmod, identifier='', value=''):
        self.__premodifier = premod
        self.__type = type
        self.__postmodifier = postmod
        self.__identifier = identifier
        self.__value = value
    def premodifier(self): return self.__premodifier
    def type(self): return self.__type
    def postmodifier(self): return self.__postmodifier
    def identifier(self): return self.__identifier
    def value(self): return self.__value
    def accept(self, visitor): visitor.visitParameter(self)

    def set_type(self, type): self.__type = type

    def __cmp__(self, other):
	"Comparison operator"
	#print "Parameter.__cmp__"
	return cmp(self.type(),other.type())
    def __str__(self):
	return "%s%s%s"%(self.__premodifier,str(self.__type),self.__postmodifier)

class Function (Declaration):
    """Function class.

  premodifier()  -- the premodifier ('oneway' etc.)
  returnType()   -- the return type
  parameters()   -- the parameters
  postmodifier() -- the postmodifier ('const' etc.)"""
    def __init__(self, file, line, main, language, type, premod, returnType, name, realname):
        Declaration.__init__(self, file, line, main, language, type, name)
	self.__realname = realname
        self.__premodifier = premod
        self.__returnType = returnType
        self.__parameters = []
        self.__postmodifier = []
        self.__exceptions = []
    def premodifier(self): return self.__premodifier
    def returnType(self): return self.__returnType
    def realname(self): return self.__realname
    def parameters(self): return self.__parameters
    def postmodifier(self): return self.__postmodifier
    def exceptions(self): return self.__exceptions
    def accept(self, visitor): visitor.visitFunction(self)

    def set_returnType(self, type): self.__type = type

    def __cmp__(self, other):
	"Recursively compares the typespec of the function"
	return ccmp(self,other) or cmp(self.parameters(), other.parameters())

class Operation (Function):
    """Operation class.
    """
    def __init__(self, file, line, main, language, type, premod, returnType, name, realname):
        Function.__init__(self, file, line, main, language, type, premod, returnType, name, realname)
    def accept(self, visitor): visitor.visitOperation(self)

class Comment :
    """Class containing information about a comment

Functions:

  text()    -- text of the comment
  __str__() -- same as text()
  file()    -- file containing the comment
  line()    -- line number in file"""

    def __init__(self, text, file, line):
        self.__text = text
        self.__file = file
        self.__line = line

    def text(self)    : return self.__text
    def __str__(self) : return self.__text
    def file(self)    : return self.__file
    def line(self)    : return self.__line
