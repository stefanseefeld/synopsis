import sys, getopt, os, os.path, string
from Synopsis import Types

def entity(type, body): return "<" + type + "> " + body + "</" + type + ">"

class TableOfContents:
    """
    generate a dictionary of all declarations which can be looked up to create
    links
    """
    def __init__(self):
        self.__toc__ = {}
    def lookup(self, name):
        n = name
        if len(n) > 2 and n[0] == ':' and n[1] == ':': n = n[2:]
        if self.__toc__.has_key(n): return self.__toc__[n]
        else: return ""
    def insert(self, name):
        n = name
        if len(n) > 2 and n[0] == ':' and n[1] == ':': n = n[2:]
        self.__toc__[n] = n

    def visitTypedef(self, typedef):
        for i in typedef.identifiers(): self.insert(i)

    def visitVariable(self, variable):
        for i in variable.identifiers(): self.insert(i)

    def visitModule(self, module):
        self.insert(module.identifier())
        for declaration in module.declarations():
            declaration.accept(self)

    def visitClass(self, clas):
        self.insert(clas.identifier())
        for declaration in clas.declarations():
            declaration.accept(self)
        for operation in clas.operations():
            operation.accept(self)

    def visitParameter(self, parameter):
        self.insert(parameter.type())

    def visitFunction(self, function):
        self.insert(function.identifier())
        for parameter in function.parameters(): parameter.accept(self)

    def visitOperation(self, operation):
        self.insert(operation.identifier())
        for parameter in operation.parameters(): parameter.accept(self)
    

class HTMLFormatter:
    """
    outputs as html.
    """
    def __init__(self, os, toc):
        self.__os = os
        self.__toc = toc
    def reference(self, ref):
        location = self.__toc.lookup(ref)
        if location != "":
            return "<a href=#" + location + ">" + ref + "</a>"
        else: return "<i>" + ref + "</i>"

    def label(self, ref):
        location = self.__toc.lookup(ref)
        if location != "":
            return "<a name=\"" + location + "\">" + ref + "</a>"
        else: return ref

    def visitTypedef(self, typedef):
        self.__os.write(entity("b", "typedef") + " " + self.reference(typedef.alias()))
        for i in typedef.identifiers(): self.__os.write(" " + self.label(i))

    def visitVariable(self, variable):
        self.__os.write(self.reference(variable.type()))
        for i in variable.identifiers():
            self.__os.write(" " + self.label(i))

    def visitModule(self, module):
        self.__os.write(entity("b", module.type()) + " " + self.label(module.identifier()) + "\n")
        self.__os.write("<blockquote>\n")
        for declaration in module.declarations():
            declaration.accept(self)
            self.__os.write("<br>\n")
        self.__os.write("</blockquote>\n")

    def visitClass(self, clas):
        self.__os.write(entity("b", clas.type()) + " " + self.label(clas.identifier()) + "\n")
        if len(clas.parents()):
            self.__os.write("<blockquote>\n")
            self.__os.write(entity("b", "parents:") + "\n")
            self.__os.write("<blockquote>\n")
            for parent in clas.parents():
                parent.accept(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
            self.__os.write("</blockquote>\n")
        if len(clas.declarations()):
            self.__os.write("<blockquote>\n")
            for d in clas.declarations():
                d.accept(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")
        if len(clas.operations()):
            self.__os.write("<blockquote>\n")
            for operation in clas.operations():
                operation.accept(self)
                self.__os.write("<br>\n")
            self.__os.write("</blockquote>\n")

    def visitInheritance(self, inheritance):
        for attribute in inheritance.attributes(): self.__os.write(attribute + " ")
        self.__os.write(self.reference(inheritance.parent().identifier()))

    def visitParameter(self, parameter):
        for m in parameter.premodifier(): self.__os.write(m + " ")
        self.__os.write(self.reference(parameter.type()) + " ")
        for m in parameter.postmodifier(): self.__os.write(m + " ")
        if len(parameter.identifier()) != 0:
            self.__os.write(parameter.identifier())
            if len(parameter.value()) != 0:
                self.__os.write(" = " + parameter.value())

    def visitFunction(self, function):
        self.__os.write(self.label(function.identifier()) + "(")
        for parameter in function.parameters(): parameter.accept(self)
        self.__os.write(")\n")

    def visitOperation(self, operation):
        for modifier in operation.premodifier(): self.__os.write(modifier + " ")
        self.__os.write(self.reference(operation.returnType()) + " ")
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.__os.write("attribute ")
        self.__os.write(self.label(operation.identifier()) + "(")
        for parameter in operation.parameters(): parameter.accept(self)
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
    toc = TableOfContents()
    for type in dictionary:
        type.accept(toc)
    formatter = HTMLFormatter(output, toc)
    for type in dictionary:
        type.accept(formatter)
        output.write("<br>\n")
