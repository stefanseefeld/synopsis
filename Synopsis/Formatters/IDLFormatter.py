from Synopsis import Types

class IDLFormatter:
    """
    outputs as idl. This is to test for features
    still missing. The output should be compatible
    with the input...
    """
    def __init__(self): pass

    def formatBaseType(self, base):
        print base.name(), " defined in ", base.file()

    def formatTypedef(self, typedef):
        print typedef.name(), " defined in ", typedef.file()

    def formatModule(self, module):
        print "module ",  module.name()
        print "{"
        for type in module.types(): type.output(self)
        print "};"

    def formatClass(self, clas):
        print "interface ",  clas.name()
        print "{"
        for type in clas.types(): type.output(self)
        for operation in clas.operations(): operation.output(self)
        print "};"

    def formatArgument(self, argument):
        if len(argument.premodifier()) != 0: print argument.premodifier(),
        print argument.type().name(),
        if len(argument.postmodifier()) != 0: print argument.postmodifier(),
        if len(argument.name()) != 0:
            print argument.name(),
            if len(argument.value()) != 0:
                print " = ", argument.value(),

    def formatFunction(self, function):
        print "function ",  function.name(), " defined in ", function.file()
        for argument in function.arguments(): argument.output(self)

    def formatOperation(self, operation):
        print operation.name(), "(",
        for argument in operation.arguments(): argument.output(self)
        print ")"
