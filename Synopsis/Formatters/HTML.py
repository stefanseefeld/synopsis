import sys, getopt, os, os.path, string
from Synopsis import Types

def entity(type, body): return "<" + type + "> " + body + "</" + type + ">"

class HTMLFormatter:
    """
    outputs as html.
    """
    def __init__(self, os):
        self.__os = os

    def formatBaseType(self, base):
        self.__os.write(base.type())
        self.__os.write(" ")
        self.__os.write(base.identifier())
        self.__os.write("\n")

    def formatTypedef(self, typedef):
        self.__os.write(entity("b", typedef.type()) + " " + typedef.identifier() + " " + typedef.alias() + "\n")

    def formatVariable(self, variable):
        self.__os.write(entity("b", variable.type()) + " " + variable.identifier() + "\n")

    def formatConst(self, const):
        self.__os.write(entity("b", const.type()) + " " + const.identifier())
        if len(const.value()) != 0: self.__os.write(" = " + const.value() + "\n")

    def formatModule(self, module):
        self.__os.write(entity("b", module.type()) + " " + module.identifier() + "\n")
        self.__os.write("<blockquote>\n")
        for type in module.types():
            type.output(self)
            self.__os.write("<br>\n")
        self.__os.write("</blockquote>\n")

    def formatClass(self, clas):
        self.__os.write(entity("b", clas.type()) + " " + clas.identifier() + "\n")
        if len(clas.parents()):
            self.__os.write("<blockquote>\n")
            self.__os.write(entity("b", "parents:") + "\n")
            self.__os.write("<blockquote>\n")
            for parent in clas.parents():
                parent.output(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
            self.__os.write("</blockquote>\n")
        if len(clas.types()):
            self.__os.write("<blockquote>\n")
            self.__os.write(entity("b", "nested types:") + "\n")
            self.__os.write("<blockquote>\n")
            for type in clas.types():
                type.output(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
            self.__os.write("</blockquote>\n")
        if len(clas.operations()):
            self.__os.write("<blockquote>\n")
            self.__os.write(entity("b", "operations:") + "\n")
            self.__os.write("<blockquote>\n")
            for operation in clas.operations():
                operation.output(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
            self.__os.write("</blockquote>\n")
        if len(clas.attributes()):
            self.__os.write("<blockquote>\n")
            self.__os.write(entity("b", "members:") + "\n")
            self.__os.write("<blockquote>\n")
            for attribute in clas.attributes():
                attribute.output(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
            self.__os.write("</blockquote>\n")

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
        self.__os.write(operation.returnType() + " ")
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
    global output
    __parseArgs(args)
    formatter = HTMLFormatter(output)
    for type in dictionary:
        type.output(formatter)
        output.write("<br>\n")
