class Type:
    """Type abstract class.
Function:
  file()            -- the file this type was defined in
  language()        -- the language this type was defined in
  type()            -- the type name in it's language ('class', 'struct' etc.)
  name()            -- the type to be defined
  output(formatter) -- visitor pattern accept."""

    def __init__(self, file, language, type, name, scope):
        self.__file  = file
        self.__language = language
        self.__type = type
        self.__name = name
        self.__scope = scope
    def file(self): return self.__file
    def language(self): return self.__language
    def type(self): return self.__type
    def name(self): return self.__name
    def scope(self): return self.__scope
    def output(self, formatter): pass

class BaseType (Type):
    def __init__(self, file, language, type, name, scope):
        Type.__init__(self, file, language, type, name, scope)
    def output(self, formatter): formatter.formatBaseType(self)

class Typedef (Type):
    def __init__(self, file, language, type, name, scope):
        Type.__init__(self, file, language, type, name, scope)
    def output(self, formatter): formatter.formatTypedef(self)

class Scope (Type):
    def __init__(self, file, language, type, name, scope):
        Type.__init__(self, file, language, type, name, scope)
        self.__types = []
    def types(self): return self.__types
    def add(self, type): self.__types.append(type)

class Module (Scope):
    def __init__(self, file, language, type, name, scope):
        Scope.__init__(self, file, language, type, name, scope)
    def output(self, formatter): formatter.formatModule(self)

class Class (Scope):
    def __init__(self, file, language, type, name, scope):
        Scope.__init__(self, file, language, type, name, scope)
        self.__operations = []
    def operations(self): return self.__operations
    def output(self, formatter): formatter.formatClass(self)

class Argument:
    def __init__(self, premod, type, postmod, name='', value=''):
        self.__premodifier = premod
        self.__type = type
        self.__postmodifier = postmod
        self.__name = name
        self.__value = value
    def premodifier(self): return self.__premodifier
    def type(self): return self.__type
    def postmodifier(self): return self.__postmodifier
    def name(self): return self.__name
    def value(self): return self.__value
    def output(self, formatter): formatter.formatArgument(self)

class Function (Type):
    def __init__(self, file, language, type, name, scope):
        Type.__init__(self, file, language, type, name, scope)
        self.__arguments = []
    def add(self, argument): self.__arguments.append(argument)
    def arguments(self): return self.__arguments
    def output(self, formatter): formatter.formatFunction(self)

class Operation (Function):
    def __init__(self, file, language, type, name, scope):
        Function.__init__(self, file, language, type, name, scope)
    def output(self, formatter): formatter.formatOperation(self)

