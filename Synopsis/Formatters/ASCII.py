import sys, getopt, os, os.path, string
from Synopsis import Type, AST, Util

class ASCIIFormatter:
    """
    outputs as ascii. This is to test for features
    still missing. The output should be compatible
    with the input...
    """
    def __init__(self, os):
        self.__indent = 0
        self.__istring = "  "
        self.__os = os
	self.__scope = []
    def indent(self):
        self.__os.write(self.__istring * self.__indent)
    def incr(self): self.__indent = self.__indent + 1
    def decr(self): self.__indent = self.__indent - 1
    def scope(self): return self.__scope
    def enterScope(self, name): self.__scope.append(name),self.incr()
    def leaveScope(self): self.__scope.pop(),self.decr()

    def formatType(self, type):
	if type is None: return '(unknown)'
	type.accept(self)
	return self.__type
    
    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type = type.name()
        
    def visitForward(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitModifier(self, type):
        aliasStr = self.formatType(type.alias())
        self.__type = "%s %s %s"%(string.join(type.premod()), aliasStr, string.join(type.postmod()))
            
    def visitParametrized(self, type):
        type.template().accept(self)
        type_save = self.__type
        parameters = []
        for p in type.parameters():
            p.accept(self)
            parameters.append(self.__type)
        self.__type = "%s<%s>"%(type_save,string.join(parameters, ", "))

    def visitTypedef(self, typedef):
        self.indent()
	dstr = ""
	# Figure out the type:
	tstr = self.formatType(typedef.alias())
	# Figure out the declarators:
	for declarator in typedef.declarators():
	    dstr = dstr + declarator.name()[-1]
	    if declarator.sizes() is None: continue
	    for size in declarator.sizes():
		dstr = dstr + "[%d]"%size
        self.__os.write("typedef %s %s;\n"%(
	    tstr, dstr
	))

    def visitModule(self, module):
        self.indent()
	self.__os.write("%s %s {\n"%(module.type(),module.name()[-1]))
        self.enterScope(module.name()[-1])
        #for type in module.types(): type.output(self)
	for declaration in module.declarations():
	    declaration.accept(self)
        self.leaveScope()
	self.indent()
	self.__os.write("}\n")

    def visitClass(self, clas):
        self.indent()
	self.__os.write("%s %s"%(clas.type(),clas.name()[-1]))
        if len(clas.parents()):
            self.__os.write(": ")
	    p = []
            for parent in clas.parents():
                p.append("%s"%(Util.ccolonName(parent.parent().name(),clas.name()),))
	    self.__os.write(string.join(p, ", "))
	self.__os.write(" {\n")
        self.enterScope(clas.name()[-1])
        #for type in clas.types(): type.output(self)
        #for operation in clas.operations(): operation.output(self)
	for declaration in clas.declarations():
	    declaration.accept(self)
        self.leaveScope()
	self.indent()
	self.__os.write("};\n")

    def visitInheritance(self, inheritance):
        for attribute in inheritance.attributes(): self.__os.write(attribute + " ")
        self.__os.write(inheritance.parent().identifier())

    def visitParameter(self, parameter):
	spacer = lambda x: str(x)+" "
	premod = string.join(map(spacer,parameter.premodifier()),'')
	parameter.type().accept(self)
	type = self.__type
	postmod = string.join(map(spacer,parameter.postmodifier()),'')
	name = ""
	value = ""
        if len(parameter.identifier()) != 0:
            name = " " + parameter.identifier()
            if len(parameter.value()) != 0:
                value = " = %s"%parameter.value()
	self.__params.append(premod + type + postmod + name + value)

    def visitFunction(self, function):
        self.__os.write(function.identifier() + "(")
        for parameter in function.parameters(): parameter.output(self)
        self.__os.write(")\n")

    def visitOperation(self, operation):
	self.indent()
        for modifier in operation.premodifier(): self.__os.write(modifier + " ")
	retStr = self.formatType(operation.returnType())
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.__os.write("attribute %s %s"%(retStr,operation.name()[-1]))
	else:
	    if operation.language() == "C++" and operation.name()[-1] in [operation.name()[-2],"~ "+operation.name()[-2]]:
		self.__os.write("%s("%operation.name()[-1])
	    else:
		self.__os.write(retStr)
		self.__os.write(" %s("%operation.name()[-1])
	    self.__params = []
	    for parameter in operation.parameters(): parameter.accept(self)
	    params = string.join(self.__params, ", ")
	    self.__os.write(params + ")")
        for modifier in operation.postmodifier(): self.__os.write(modifier + " ")
        self.__os.write(";\n")

    def visitVariable(self, var):
	var.vtype().accept(self)
	vtype = self.__type
	names = map(lambda x: x.name()[-1], var.declarators())
	names = string.join(names, ", ")
	self.indent()
	self.__os.write("%s %s;\n"%(vtype,names))

    def visitEnum(self, enum):
	self.indent()
	istr = self.__istring * (self.__indent+1)
	self.__os.write("enum %s {\n%s"%(enum.name()[-1],istr))
	self.__enumers = []
	for enumer in enum.enumerators():
	    enumer.accept(self)
	self.__os.write(string.join(self.__enumers, ",\n%s"%istr))
	self.__os.write("\n")
	self.indent()
	self.__os.write("}\n")

    def visitEnumerator(self, enumer):
	if enumer.value() == "":
	    self.__enumers.append("%s"%enumer.name()[-1])
	else:
	    self.__enumers.append("%s = %s"%(enumer.name()[-1], enumer.value()))

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

def format(types, declarations, args):
    __parseArgs(args)
    formatter = ASCIIFormatter(output)
    #for type in types:
    #    type.output(formatter)
    for declaration in declarations:
	declaration.accept(formatter)
