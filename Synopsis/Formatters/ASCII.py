import sys, getopt, os, os.path, string
from Synopsis import Type, AST

class ASCIIFormatter:
    """
    outputs as ascii. This is to test for features
    still missing. The output should be compatible
    with the input...
    """
    def __init__(self, os):
        self.__indent = 0
        self.__istring = " "
        self.__os = os
    def indent(self):
        self.__os.write(self.__istring * self.__indent)
    def incr(self): self.__indent = self.__indent + 1
    def decr(self): self.__indent = self.__indent - 1
    def formatBaseType(self, base):
        self.indent()
        self.__os.write(base.type())
        self.__os.write(" ")
        self.__os.write(base.identifier())
        self.__os.write("\n")

    def formatTypedef(self, typedef):
        self.indent()
        self.__os.write(typedef.type())
        self.__os.write(" ")
        self.__os.write(typedef.identifier())
        self.__os.write("\n")

    def formatModule(self, module):
        self.indent()
        self.__os.write(module.type())
        self.__os.write(" ")
        self.__os.write(module.identifier())
        self.__os.write("\n")
        self.incr()
        for type in module.types(): type.output(self)
        self.decr()

    def formatClass(self, clas):
        self.indent()
        self.__os.write(clas.type())
        self.__os.write(" ")
        self.__os.write(clas.identifier())
        self.__os.write("\n")
        self.incr()
        if len(clas.parents()):
            self.indent()
            self.__os.write("parents:\n")
            self.incr()
            for parent in clas.parents():
                self.indent()
                parent.output(self)
                self.__os.write("\n")
            self.decr()
        for type in clas.types(): type.output(self)
        for operation in clas.operations(): operation.output(self)
        self.decr()

    def formatInheritance(self, inheritance):
        for attribute in inheritance.attributes(): self.__os.write(attribute + " ")
        self.__os.write(inheritance.parent().identifier())

    def formatParameter(self, parameter):
        for m in parameter.premodifier(): self.__os.write(m + " ")
        self.__os.write(parameter.type() + " ")
        for m in parameter.postmodifier(): self.__os.write(m + " ")
        if len(parameter.identifier()) != 0:
            self.__os.write(parameter.identifier())
            if len(parameter.value()) != 0:
                self.__os.write(" = " + parameter.value())

    def formatFunction(self, function):
        self.__os.write(function.identifier() + "(")
        for parameter in function.parameters(): parameter.output(self)
        self.__os.write(")\n")

    def formatOperation(self, operation):
        for modifier in operation.premodifier(): self.__os.write(modifier + " ")
        #self.__os.write(operation.returnType())
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.__os.write("attribute ")
        self.__os.write(operation.identifier() + "(")
        for parameter in operation.parameters(): parameter.output(self)
        self.__os.write(")")
        for modifier in operation.postmodifier(): self.__os.write(modifier + " ")
        self.__os.write("\n")

def __parseArgs(args):
    global output
    output = sys.stdout
    try:
        opts,remainder = getopt.getopt(args, "o:")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt

        if o == "-o":
            output = open(a, "w")

def format(dictionary, args):
    __parseArgs(args)
    formatter = ASCIIFormatter(output)
    for type in dictionary:
        type.output(formatter)
