from omniidl import idlast, idltype, idlvisitor, idlutil
import _omniidl
import sys, getopt, os, os.path, string
from Synopsis import Type, AST, Util

class TypeTranslator (idlvisitor.TypeVisitor):
    """maps idltype objects to Synopsis.Type objects in a Type.Dictionary"""
    def __init__(self, types):
        self.types = types
        self.__result = None
        self.__basetypes = {
            idltype.tk_void:       "void",
            idltype.tk_short:      "short",
            idltype.tk_long:       "long",
            idltype.tk_ushort:     "unsigned short",
            idltype.tk_ulong:      "unsigned long",
            idltype.tk_float:      "float",
            idltype.tk_double:     "double",
            idltype.tk_boolean:    "boolean",
            idltype.tk_char:       "char",
            idltype.tk_octet:      "octet",
            idltype.tk_any:        "any",
            idltype.tk_TypeCode:   "CORBA::TypeCode",
            idltype.tk_Principal:  "CORBA::Principal",
            idltype.tk_longlong:   "long long",
            idltype.tk_ulonglong:  "unsigned long long",
            idltype.tk_longdouble: "long double",
            idltype.tk_wchar:      "wchar"
            }

    def internalize(self, idltype):
        idltype.accept(self)
        return self.__result

    def add(self, name, type): self.types[name] = type
    def get(self, name): return self.types[name]

    def visitBaseType(self, idltype):
        type = Type.Base("IDL", (self.__basetypes[idltype.kind()],))
        self.types[type.name()] = type
        self.__result = type.name()

    def visitStringType(self, idltype):
        if not self.types.has_key(["string"]):
            self.types[["string"]] = Type.Base("IDL", ("string",))
        self.__result = ["string"]
        #if idltype.bound() == 0:
        #    self.__result_type = "string"
        #else:
        #    self.__result_type = "string<" + str(type.bound()) + ">"

    def visitWStringType(self, idltype):
        if not self.types.has_key(["wstring"]):
            self.types[["wstring"]] = Type.Base("IDL", ("wstring",))
        self.__result = ["wstring"]
        #if type.bound() == 0:
        #    self.__result_type = "wstring"
        #else:
        #    self.__result_type = "wstring<" + str(type.bound()) + ">"

    def visitSequenceType(self, idltype):
        if not self.types.has_key(["sequence"]):
            self.types[["sequence"]] = Type.Base("IDL", ("sequence",))
        idltype.seqType().accept(self)
        ptype = self.types[self.__result]
        #if type.bound() == 0:
        #    self.__result_type = "sequence<" + self.__result_type + ">"
        #else:
        #    self.__result_type = "sequence<" + self.__result_type + ", " +\
        #                         str(type.bound()) + ">"
        type = Type.Parametrized("IDL", self.types[["sequence"]], [ptype])
        name  = ["sequence<" + Util.ccolonName(ptype.name()) + ">"]
        self.types[name] = type
        self.__result = name
        
    def visitDeclaredType(self, idltype):
        self.__result = idltype.decl().scopedName()

class ASTTranslator (idlvisitor.AstVisitor):
    def __init__(self, declarations, types):
        self.declarations = declarations
        self.__types = types
        self.__scope = []
        self.__operation = None
        self.__enum = None
        
    def scope(self): return self.__scope[-1].name()
    def addDeclaration(self, declaration): self.__scope[-1].declarations().append(declaration)
    def addType(self, name, type):
	if self.__types.types.has_key(name):
	    if isinstance(self.__types.get(name), Type.Forward):
		self.__types.add(name, type)
	    else:
		pass
	    return
	self.__types.add(name, type)
    def visitAST(self, node):
        self.__scope.append(AST.Scope(node.file(), 0, 1, "IDL", "file", []))
        # add an 'Object' Type to the Type Dictionary. Don't declare it in the AST since
        # there is no corresponding declaration
        object = AST.Class("<built in>", 0, 0, "IDL", "interface", ["CORBA", "Object"])
        self.addType(["CORBA", "Object"], Type.Declared("IDL", ["CORBA", "Object"], object))
        for n in node.declarations():
            n.accept(self)
        for d in self.__scope[-1].declarations():
            self.declarations.append(d)

    def visitModule(self, node):
	name = list(self.scope())
        name.append(node.identifier())
        module = AST.Module(node.file(), node.line(), node.mainFile(), "IDL", "module", name)
        self.addDeclaration(module)
        self.__scope.append(module)
        self.addType(name, Type.Declared("IDL", name, module))
        for c in node.comments():
            module.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for n in node.definitions():
            n.accept(self)
        self.__scope.pop()
        
    def visitInterface(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        clas = AST.Class(node.file(), node.line(), node.mainFile(), "IDL", "interface", name)
        self.addDeclaration(clas)
        self.__scope.append(clas)
        self.addType(name, Type.Declared("IDL", name, clas))
        for c in node.comments():
            clas.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for i in node.inherits():
            parent = self.__types.get(i.scopedName())
            clas.parents().append(AST.Inheritance("", parent.declaration(), []))
        for c in node.contents(): c.accept(self)
        self.__scope.pop()
        
    def visitForward(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        self.__result_declarator = AST.Forward(node.file(), node.line(), node.mainFile(), "IDL", "interface", name)
        self.addType(name, Type.Forward("IDL", name))
        
    def visitConst(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        type = self.__types.internalize(node.constType())
        if node.constType().kind() == idltype.tk_enum:
            value = "::" + idlutil.ccolonName(node.value().scopedName())
        else:
            value = str(node.value())
        const = AST.Const(node.file(), node.line(), node.mainFile(), "IDL", "const",
                          self.__types.get(type), name, value)
        self.addDeclaration(const)
        for c in node.comments():
            const.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        
    def visitDeclarator(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        self.__result_declarator = AST.Declarator(node.file(), node.line(), node.mainFile(), "IDL", name, node.sizes())
        #self.addType(self.__result_declarator.name(), Type.Declared("IDL", self.__result_declarator.name(), self.__result_declarator))
        for c in node.comments():
            self.__result_declarator.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        
    def visitTypedef(self, node):
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.memberType().decl().accept(self)
        name = self.__types.internalize(node.aliasType())
	comments = []
        for c in node.comments():
            comments.append(AST.Comment(c.text(), c.file(), c.line()))
        for d in node.declarators():
            d.accept(self)
            decl = self.__result_declarator
	    typedef = AST.Typedef(node.file(), node.line(), node.mainFile(), "IDL", "typedef",
				  decl.name(), self.__types.get(name), node.constrType(), decl)
	    typedef.comments().extend(comments + decl.comments())
	    self.addType(typedef.name(), Type.Declared("IDL", typedef.name(), typedef))
	    self.addDeclaration(typedef)

    def visitMember(self, node):
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.memberType().decl().accept(self)
        name = self.__types.internalize(node.memberType())
	comments = []
        for c in node.comments():
            comments.append(AST.Comment(c.text(), c.file(), c.line()))
        for d in node.declarators():
            d.accept(self)
	    decl = self.__result_declarator
	    member = AST.Variable(node.file(), node.line(), node.mainFile(), "IDL", "variable",
				  decl.name(), self.__types.get(name), node.constrType(), decl)
	    member.comments().extend(comments + decl.comments())
	    self.addType(member.name(), Type.Declared("IDL", member.name(), member))
	    self.addDeclaration(member)

    def visitStruct(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        struct = AST.Class(node.file(), node.line(), node.mainFile(), "IDL", "struct", name)
        self.addDeclaration(struct)
        self.addType(name, Type.Declared("IDL", name, struct))
        for c in node.comments():
            struct.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        self.__scope.append(struct)
        for member in node.members(): member.accept(self)
        self.__scope.pop()
        
    def visitException(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        exc = AST.Class(node.file(), node.line(), node.mainFile(), "IDL", "exception", name)
        self.addDeclaration(exc)
        self.addType(name, Type.Declared("IDL", name, exc))
        self.__scope.append(exc)
        for c in node.comments():
            exc.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for member in node.members(): member.accept(self)
        self.__scope.pop()
    
    #    def visitCaseLabel(self, node):    return
    def visitUnionCase(self, node):
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.caseType().decl().accept(self)
        type = self.__types.internalize(node.caseType())
        node.declarator().accept(self)
        name = list(self.scope())
        name.append(self.__result_declarator.name()[-1])
        self.__scope[-1].declarations().append(AST.Operation(node.file(), node.line(), node.mainFile(), "IDL", "case",
                                                             [], self.__types.get(type), name, list(name)))
    def visitUnion(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        clas = AST.Class(node.file(), node.line(), node.mainFile(), "IDL", "union", name)
        self.addDeclaration(clas)
        self.__scope.append(clas)
        self.addType(name, Type.Declared("IDL", name, clas))
        for c in node.comments():
            clas.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for c in node.cases(): c.accept(self)
        self.__scope.pop()
        
    def visitEnumerator(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        enum = AST.Enumerator(node.file(), node.line(), node.mainFile(), "IDL", name, "")
        self.addType(name, Type.Declared("IDL", name, enum))
        self.__enum.enumerators().append(enum)

    def visitEnum(self, node):
        name = list(self.scope())
        name.append(node.identifier())
        self.__enum = AST.Enum(node.file(), node.line(), node.mainFile(), "IDL", name, [])
        self.addDeclaration(self.__enum)
        self.addType(name, Type.Declared("IDL", name, self.__enum))
        for c in node.comments():
            self.__enum.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for enumerator in node.enumerators(): enumerator.accept(self)
        self.__enum = None
        
    def visitAttribute(self, node):
        pre = []
        if node.readonly(): pre.append("readonly")
        type = self.__types.internalize(node.attrType())
        name = list(self.scope())
        name.append(node.identifiers()[0])
        attr = AST.Operation(node.file(), node.line(), node.mainFile(), "IDL", "attribute",
                             pre, self.__types.get(type), name, list(name))
        self.__scope[-1].declarations().append(attr)
        for c in node.comments():
            attr.comments().append(AST.Comment(c.text(), c.file(), c.line()))

    def visitParameter(self, node):
        operation = self.__operation
        pre = []
        if node.direction() == 0: pre.append("in")
        elif node.direction() == 1: pre.append("out")
        else: pre.append("inout")
        post = []
        name = self.__types.internalize(node.paramType())
        operation.parameters().append(AST.Parameter(pre, self.__types.get(name), post, node.identifier()))
    
    def visitOperation(self, node):
        pre = []
        if node.oneway(): pre.append("oneway")
        returnType = self.__types.internalize(node.returnType())
        name = list(self.scope())
        name.append(node.identifier())
        self.__operation = AST.Operation(node.file(), node.line(), 1, "IDL", "operation", pre, self.__types.get(returnType), name, list(name))
        for c in node.comments():
            self.__operation.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for p in node.parameters(): p.accept(self)
        for e in node.raises():
            exception = self.__types.get(e.scopedName())
            self.__operation.exceptions().append(exception)
            
        self.addDeclaration(self.__operation)
        self.__operation = None
        
#    def visitNative(self, node):       return
#    def visitStateMember(self, node):  return
#    def visitFactory(self, node):      return
#    def visitValueForward(self, node): return
#    def visitValueBox(self, node):     return
#    def visitValueAbs(self, node):     return
#    def visitValue(self, node):        return

def __parseArgs(args):
    global preprocessor_args

    preprocessor_args = []
    try:
        opts,remainder = getopt.getopt(args, "I:kK")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt

        if o == "-I":
            preprocessor_args.append("-I" + a)
        elif o == "-k":
            preprocessor_args.append("-C")
            _omniidl.keepComments(0)
        elif o == "-K":
            preprocessor_args.append("-C")
            _omniidl.keepComments(1)

def parse(file, args, typedict, astdict):
    global preprocessor_args
    __parseArgs(args)
    if hasattr(_omniidl, "__file__"):
        preprocessor_path = os.path.dirname(_omniidl.__file__)
    else:
        preprocessor_path = os.path.dirname(sys.argv[0])
    preprocessor      = os.path.join(preprocessor_path, "omnicpp")
    preprocessor_cmd  = preprocessor + " -lang-c++ -undef -D__OMNIIDL__=" + _omniidl.version
    preprocessor_cmd = preprocessor_cmd + " " + string.join(preprocessor_args, " ") + " " + file
    fd = os.popen(preprocessor_cmd, "r")

    _omniidl.noForwardWarning()
    tree = _omniidl.compile(fd)
    if tree == None:
        sys.stderr.write("omni: Error parsing " + file + "\n")
        sys.exit(1)
    types = TypeTranslator(typedict)
    ast = ASTTranslator(astdict, types)
    tree.accept(ast)
    return ast.declarations, types.types
