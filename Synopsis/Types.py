class Declaration:
    """Declaration base class

  file()            -- the file this type was defined in
  line()            -- the line this type was defined in
  language()        -- the language this type was defined in
  description()     -- a list of strings describing this declaration
  accept(visitor) -- visitor pattern accept."""

    def __init__(self, file, line, language):
        self.__file  = file
        self.__line  = line
        self.__language = language
        self.__description = []
    def file(self): return self.__file
    def line(self): return self.__line
    def language(self): return self.__language
    def description(self): return self.__description
    def accept(self, visitor): visitor.visitDeclaration(self)

class Variable (Declaration):
    """Variable declaration class

  metatype()        -- what kind of variable ('variable', 'const', 'enum' etc.)
  type()            -- the type in it's lanugage (float, short, string etc.)
  scope()           -- the scope within which the type exists
  identifiers()     -- the names being declared"""
    def __init__(self, file, line, language, metatype, type, scope, identifiers, value):
        Declaration.__init__(self, file, line, language)
        self.__metatype = metatype
        self.__type = type
        self.__scope = scope
        self.__identifiers = identifiers
        self.__value = value
    def metatype(self): return self.__metatype
    def type(self): return self.__type
    def scope(self): return self.__scope
    def identifiers(self): return self.__identifiers
    def value(self): return self.__value
    def accept(self, visitor): visitor.visitVariable(self)

class Type (Declaration):
    """Type class.

  type()            -- the type name in it's language ('class', 'struct' etc.)
  identifier()      -- the identifier of the type
  scope()           -- list of strings forming the scope of this type"""
    def __init__(self, file, line, language, type, identifier, scope):
        Declaration.__init__(self, file, line, language)
        self.__type = type
        self.__identifier = identifier
        self.__scope = scope
    def type(self): return self.__type
    def identifier(self): return self.__identifier
    def scope(self): return self.__scope
    def accept(self, visitor): pass

class Typedef (Declaration):
    """Typedef class.

  alias()           -- the type object referenced by this alias
  scope()           -- list of strings forming the scope of this type
  identifiers()     -- the new types being declared"""
    def __init__(self, file, line, language, alias, scope, identifiers):
        Declaration.__init__(self, file, line, language)
        self.__alias = alias
        self.__scope = scope
        self.__identifiers = identifiers
    def alias(self): return self.__alias
    def scope(self): return self.__scope
    def identifiers(self): return self.__identifiers
    def accept(self, visitor): visitor.visitTypedef(self)

class Scope (Type):
    """Scope class.

  declarations()    -- the declarations inside this scope"""
    def __init__(self, file, line, language, type, identifier, scope):
        Type.__init__(self, file, line, language, type, identifier, scope)
        self.__declarations = []
    def declarations(self): return self.__declarations

class Module (Scope):
    """Module class.
    """
    def __init__(self, file, line, language, type, identifier, scope):
        Scope.__init__(self, file, line, language, type, identifier, scope)
    def accept(self, visitor): visitor.visitModule(self)

class Class (Scope):
    """Class class.

  parents()    -- the parents this Class is derived from
  operations() -- the operations declared for this class"""
    def __init__(self, file, line, language, type, identifier, scope):
        Scope.__init__(self, file, line, language, type, identifier, scope)
        self.__parents = []
        self.__operations = []
    def parents(self): return self.__parents
    def operations(self): return self.__operations
    def accept(self, visitor): visitor.visitClass(self)

class Inheritance:
    """Inheritance class.

  type()       -- the type of inheritance ('implements', 'extends' etc.)
  parent()     -- the parent Class
  attributes() -- the attributes of this inheritance (virtual, public etc."""
    def __init__(self, type, parent, attributes):
        self.__type = type
        self.__parent = parent
        self.__attributes = attributes
    def type(self): return self.__type
    def parent(self): return self.__parent
    def attributes(self): return self.__attributes
    def accept(self, visitor): visitor.visitInheritance(self)

class Parameter:
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

class Function (Type):
    """Function class.

  premodifier()  -- the premodifier ('oneway' etc.)
  returnType()   -- the return type
  parameters()   -- the parameters
  postmodifier() -- the postmodifier ('const' etc.)"""
    def __init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod):
        Type.__init__(self, file, line, language, type, identifier, scope)
        self.__premodifier = premod
        self.__returnType = returnType
        self.__parameters = []
        self.__postmodifier = postmod
    def premodifier(self): return self.__premodifier
    def returnType(self): return self.__returnType
    def parameters(self): return self.__parameters
    def postmodifier(self): return self.__postmodifier
    def accept(self, visitor): visitor.visitFunction(self)

class Operation (Function):
    """Operation class.
    """
    def __init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod):
        Function.__init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod)
    def accept(self, visitor): visitor.visitOperation(self)

