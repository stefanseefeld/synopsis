class Type:
    """Type abstract class.
Function:
  file()            -- the file this type was defined in
  line()            -- the line this type was defined in
  language()        -- the language this type was defined in
  type()            -- the type name in it's language ('class', 'struct' etc.)
  identifier()      -- the identifier of the type
  scope()           -- list of strings forming the scope of this type
  output(formatter) -- visitor pattern accept."""

    def __init__(self, file, line, language, type, identifier, scope):
        self.__file  = file
        self.__line  = line
        self.__language = language
        self.__type = type
        self.__identifier = identifier
        self.__scope = scope
    def file(self): return self.__file
    def language(self): return self.__language
    def type(self): return self.__type
    def identifier(self): return self.__identifier
    def scope(self): return self.__scope
    def output(self, formatter): pass

class BaseType (Type):
    def __init__(self, file, line, language, type, identifier, scope):
        Type.__init__(self, file, line, language, type, identifier, scope)
    def output(self, formatter): formatter.formatBaseType(self)

class Typedef (Type):
    def __init__(self, file, line, language, type, identifier, scope, alias):
        Type.__init__(self, file, line, language, type, identifier, scope)
        self.__alias = alias
    def alias(self): return self.__alias
    def output(self, formatter): formatter.formatTypedef(self)

class Variable:
    def __init__(self, file, line, language, metatype, type, scope, identifier, value):
        self.__file  = file
        self.__line  = line
        self.__language = language
        self.__metatype = metatype
        self.__type = type
        self.__scope = scope
        self.__identifier = identifier
        self.__value = value
    def file(self): return self.__file
    def line(self): return self.__line
    def language(self): return self.__language
    def metatype(self): return self.__metatype
    def type(self): return self.__type
    def scope(self): return self.__scope
    def identifier(self): return self.__identifier
    def value(self): return self.__value
    def output(self, formatter): formatter.formatVariable(self)

class Const (Variable):
    def __init__(self, file, line, language, metatype, type, scope, identifier, value):
        Variable.__init__(self, file, line, language, metatype, type, scope, identifier, value)
    def output(self, formatter): formatter.formatConst(self)

class Scope (Type):
    def __init__(self, file, line, language, type, identifier, scope):
        Type.__init__(self, file, line, language, type, identifier, scope)
        self.__types = []
        self.__attributes = []
    def types(self): return self.__types
    def attributes(self): return self.__attributes

class Module (Scope):
    def __init__(self, file, line, language, type, identifier, scope):
        Scope.__init__(self, file, line, language, type, identifier, scope)
    def output(self, formatter): formatter.formatModule(self)

class Class (Scope):
    def __init__(self, file, line, language, type, identifier, scope):
        Scope.__init__(self, file, line, language, type, identifier, scope)
        self.__parents = []
        self.__operations = []
    def parents(self): return self.__parents
    def operations(self): return self.__operations
    def output(self, formatter): formatter.formatClass(self)

class Inheritance:
    def __init__(self, type, parent, attributes):
        self.__type = type
        self.__parent = parent
        self.__attributes = attributes
    def type(self): return self.__type
    def parent(self): return self.__parent
    def attributes(self): return self.__attributes
    def output(self, formatter): formatter.formatInheritance(self)

class Parameter:
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
    def output(self, formatter): formatter.formatParameter(self)

class Function (Type):
    def __init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod):
        Type.__init__(self, file, line, language, type, identifier, scope)
        self.__premodifier = premod
        self.__returnType = returnType
        self.__parameters = []
        self.__postmodifier = postmod
    def premodifier(self): return self.__premodifier
    def postmodifier(self): return self.__postmodifier
    def returnType(self): return self.__returnType
    def parameters(self): return self.__parameters
    def output(self, formatter): formatter.formatFunction(self)

class Operation (Function):
    def __init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod):
        Function.__init__(self, file, line, language, type, premod, returnType, identifier, scope, postmod)
    def output(self, formatter): formatter.formatOperation(self)

