import sys, getopt, os, os.path, string
from Synopsis.Core import Type, AST, Util

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
	self.__axs = AST.DEFAULT
	self.__axs_stack = []
	self.__axs_string = ('default:\n','public:\n','protected:\n','private:\n')
    def indent(self):
        self.__os.write(self.__istring * self.__indent)
    def incr(self): self.__indent = self.__indent + 1
    def decr(self): self.__indent = self.__indent - 1
    def scope(self): return self.__scope
    def enterScope(self, name): self.__scope.append(name),self.incr()
    def leaveScope(self): self.__scope.pop(),self.decr()
    def write(self, text): self.__os.write(text)

    def formatType(self, type):
	if type is None: return '(unknown)'
	type.accept(self)
	return self.__type
    
    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type = Util.ccolonName(type.name())
        
    def visitForward(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type = Util.ccolonName(type.name(), self.scope())
        
    def visitModifier(self, type):
        aliasStr = self.formatType(type.alias())
	premod = map(lambda x:x+" ", type.premod())
        self.__type = "%s%s%s"%(string.join(premod,''), aliasStr, string.join(type.postmod(),''))
            
    def visitParametrized(self, type):
	temp = self.formatType(type.template())
	params = map(self.formatType, type.parameters())
        self.__type = "%s<%s>"%(temp,string.join(params, ", "))

    def visitFunctionType(self, type):
	ret = self.formatType(type.returnType())
	params = map(self.formatType, type.parameters())
	self.__type = "%s(%s)(%s)"%(ret,string.join(type.premod(),''),string.join(params,", "))

    def visitTemplate(self, type):
	self.visitDeclared(type)
	self.__type = "template<"+string.join(map(self.formatType, type.parameters()),",")+">"+self.__type
	
    ### AST visitor

    def visitDeclaration(self, decl):
	axs = decl.accessability()
	if axs != self.__axs:
	    self.decr(); self.indent(); self.incr()
	    self.write(self.__axs_string[axs])
	    self.__axs = axs
	for comment in decl.comments():
	    self.indent()
	    self.write(comment.text())
	    self.write("\n")

    def visitTypedef(self, typedef):
	self.visitDeclaration(typedef)
        self.indent()
	dstr = ""
	# Figure out the type:
	alias = self.formatType(typedef.alias())
	# Figure out the declarators:
	# for declarator in typedef.declarators():
	#     dstr = dstr + declarator.name()[-1]
	#     if declarator.sizes() is None: continue
	#     for size in declarator.sizes():
	# 	dstr = dstr + "[%d]"%size
        self.write("typedef %s %s;\n"%(
	    alias, typedef.name()[-1]
	))

    def visitModule(self, module):
	self.visitDeclaration(module)
        self.indent()
	self.write("%s %s {\n"%(module.type(),module.name()[-1]))
        self.enterScope(module.name()[-1])
        #for type in module.types(): type.output(self)
	for declaration in module.declarations():
	    declaration.accept(self)
        self.leaveScope()
	self.indent()
	self.write("}\n")

    def visitMetaModule(self, module):
	self.visitDeclaration(module)
	for decl in module.module_declarations():
	    self.visitDeclaration(decl)
	# since no comments:
	self.visitModule(module)

    def visitClass(self, clas):
	self.visitDeclaration(clas)
        self.indent()
	self.write("%s %s"%(clas.type(),clas.name()[-1]))
        if len(clas.parents()):
            self.write(": ")
	    p = []
            for parent in clas.parents():
		p.append(self.formatType(parent.parent()))
                #p.append("%s"%(Util.ccolonName(parent.parent().name(),clas.name()),))
	    self.write(string.join(p, ", "))
	self.write(" {\n")
        self.enterScope(clas.name()[-1])
	self.__axs_stack.append(self.__axs)
	if clas.type() == 'struct': self.__axs = AST.PUBLIC
	elif clas.type() == 'class': self.__axs = AST.PRIVATE
	else: self.__axs = AST.DEFAULT
        #for type in clas.types(): type.output(self)
        #for operation in clas.operations(): operation.output(self)
	for declaration in clas.declarations():
	    declaration.accept(self)
	self.__axs = self.__axs_stack.pop()
        self.leaveScope()
	self.indent()
	self.write("};\n")

    def visitInheritance(self, inheritance):
        for attribute in inheritance.attributes(): self.write(attribute + " ")
        self.write(inheritance.parent().identifier())

    def visitParameter(self, parameter):
	spacer = lambda x: str(x)+" "
	premod = string.join(map(spacer,parameter.premodifier()),'')
	type = self.formatType(parameter.type())
	postmod = string.join(map(spacer,parameter.postmodifier()),'')
	name = ""
	value = ""
        if len(parameter.identifier()) != 0:
            name = " " + parameter.identifier()
            if len(parameter.value()) != 0:
                value = " = %s"%parameter.value()
	self.__params.append(premod + type + postmod + name + value)

    def visitFunction(self, function):
	self.visitOperation(function)

    def visitOperation(self, operation):
	self.visitDeclaration(operation)
	self.indent()
        for modifier in operation.premodifier(): self.write(modifier + " ")
	retStr = self.formatType(operation.returnType())
	name = operation.realname()
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.write("attribute %s %s"%(retStr,name[-1]))
	else:
	    if operation.language() == "C++" and len(name)>1 and name[-1] in [name[-2],"~"+name[-2]]:
		self.write("%s("%name[-1])
	    else:
		self.write(retStr)
		self.write(" %s("%name[-1])
	    self.__params = []
	    for parameter in operation.parameters(): parameter.accept(self)
	    params = string.join(self.__params, ", ")
	    self.write(params + ")")
        for modifier in operation.postmodifier(): self.write(modifier + " ")
        self.write(";\n")

    def visitVariable(self, var):
	self.visitDeclaration(var)
	name = var.name
	self.indent()
	self.write("%s %s;\n"%(self.formatType(var.vtype()),var.name()[-1]))

    def visitEnum(self, enum):
	self.visitDeclaration(enum)
	self.indent()
	istr = self.__istring * (self.__indent+1)
	self.write("enum %s {\n%s"%(enum.name()[-1],istr))
	self.__enumers = []
	for enumer in enum.enumerators():
	    enumer.accept(self)
	self.write(string.join(self.__enumers, ",\n%s"%istr))
	self.write("\n")
	self.indent()
	self.write("}\n")

    def visitEnumerator(self, enumer):
	for comment in enumer.comments():
	    self.__enumers.append(comment.text())
	if enumer.value() == "":
	    self.__enumers.append("%s"%enumer.name()[-1])
	else:
	    self.__enumers.append("%s = %s"%(enumer.name()[-1], enumer.value()))
    
    def visitConst(self, const):
	self.visitDeclaration(const)
	ctype = self.formatType(const.ctype())
	self.indent()
	self.__os.write("%s %s = %s;\n"%(ctype,const.name()[-1],const.value()))


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

def print_types(types):
    keys = types.keys()
    keys.sort()
    for name in keys:
	type = types[name]
	clas = type.__class__
	if isinstance(type, Type.Declared):
	    clas = type.declaration().__class__
	try:
	    print "%s\t%s"%(string.split(clas.__name__,'.')[-1], Util.ccolonName(name))
	except:
	    print "name ==",name
	    raise

def format(types, declarations, args):
    __parseArgs(args)
    formatter = ASCIIFormatter(output)
    #for type in types:
    #    type.output(formatter)
    for declaration in declarations:
	declaration.accept(formatter)

    # print_types(types)
