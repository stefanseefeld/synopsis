# $Id: omni.py,v 1.19 2001/01/25 18:27:47 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: omni.py,v $
# Revision 1.19  2001/01/25 18:27:47  stefan
# added Type.Array type and removed AST.Declarator. Adjusted the IDL parser to that.
#
# Revision 1.18  2001/01/22 19:54:41  stefan
# better support for help message
#
# Revision 1.17  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#

from omniidl import idlast, idltype, idlvisitor, idlutil
import _omniidl
import sys, getopt, os, os.path, string
from Synopsis.Core import Type, AST, Util

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
    def __init__(self, declarations, types, mainfile_only):
        self.declarations = declarations
        self.__mainfile_only = mainfile_only
        self.__types = types
        self.__scope = []
        self.__operation = None
        self.__enum = None
        
    def scope(self): return self.__scope[-1].name()
    def addDeclaration(self, declaration): self.__scope[-1].declarations().append(declaration)
    def addType(self, name, type):
	if self.__types.types.has_key(name):
	    if isinstance(self.__types.get(name), Type.Unknown):
		self.__types.add(name, type)
	    else:
		pass
	    return
	self.__types.add(name, type)
    def getType(self, name): return self.__types.get(name)
    def visitAST(self, node):
        self.__scope.append(AST.Scope(node.file(), 0, "IDL", "file", []))
        # add an 'Object' Type to the Type Dictionary. Don't declare it in the AST since
        # there is no corresponding declaration
        object = AST.Class("<built in>", 0, "IDL", "interface", ["CORBA", "Object"])
        self.addType(["CORBA", "Object"], Type.Declared("IDL", ["CORBA", "Object"], object))
        for n in node.declarations():
            n.accept(self)
        for d in self.__scope[-1].declarations():
            self.declarations.append(d)

    def visitModule(self, node):
        if self.__mainfile_only and not node.mainFile(): return
	name = list(self.scope())
        name.append(node.identifier())
        module = AST.Module(node.file(), node.line(), "IDL", "module", name)
        self.addDeclaration(module)
        self.__scope.append(module)
        self.addType(name, Type.Declared("IDL", name, module))
        for c in node.comments():
            module.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for n in node.definitions():
            n.accept(self)
        self.__scope.pop()
        
    def visitInterface(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        clas = AST.Class(node.file(), node.line(), "IDL", "interface", name)
        self.addDeclaration(clas)
        self.__scope.append(clas)
        self.addType(name, Type.Declared("IDL", name, clas))
        for c in node.comments():
            clas.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for i in node.inherits():
            parent = self.getType(i.scopedName())
            clas.parents().append(AST.Inheritance("", parent, []))
        for c in node.contents(): c.accept(self)
        self.__scope.pop()
        
    def visitForward(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        forward = AST.Forward(node.file(), node.line(), "IDL", "interface", name)
        self.addDeclaration(forward)
        self.addType(name, Type.Unknown("IDL", name))
        
    def visitConst(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        type = self.__types.internalize(node.constType())
        if node.constType().kind() == idltype.tk_enum:
            value = "::" + idlutil.ccolonName(node.value().scopedName())
        else:
            value = str(node.value())
        const = AST.Const(node.file(), node.line(), "IDL", "const",
                          self.getType(type), name, value)
        self.addDeclaration(const)
        for c in node.comments():
            const.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        
    def visitTypedef(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.memberType().decl().accept(self)
        type = self.__types.internalize(node.aliasType())
	comments = []
        for c in node.comments():
            comments.append(AST.Comment(c.text(), c.file(), c.line()))
        for d in node.declarators():
            # reinit the type for this declarator, as each declarator of
            # a single typedef declaration can have a different type. *sigh*
            dtype = type
            if d.sizes():
                array = Type.Array("IDL", self.getType(type), d.sizes())
                dtype = map(None, type[:-1])
                dtype.append(type[-1] + string.join(map(lambda s:"["+ str(s) +"]", d.sizes()),''))
                self.addType(dtype, array)
            dname = list(self.scope())
            dname.append(d.identifier())
	    typedef = AST.Typedef(node.file(), node.line(), "IDL", "typedef", dname, self.getType(dtype), node.constrType())
	    typedef.comments().extend(comments)
            for c in d.comments():
                typedef.comments().append(AST.Comment(c.text(), c.file(), c.line()))
	    self.addType(typedef.name(), Type.Declared("IDL", typedef.name(), typedef))
	    self.addDeclaration(typedef)

    def visitMember(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.memberType().decl().accept(self)
        type = self.__types.internalize(node.memberType())
	comments = []
        for c in node.comments():
            comments.append(AST.Comment(c.text(), c.file(), c.line()))
        for d in node.declarators():
            # reinit the type for this declarator, as each declarator of
            # a single typedef declaration can have a different type. *sigh*
            dtype = type
            if d.sizes():
                array = Type.Array("IDL", self.getType(type), node.sizes())
                dtype = type[:-1]
                dtype.append(type[-1] + string.join(map(lambda s:"["+s+"]", d.sizes()),''))
                self.addType(dtype, array)
            dname = list(self.scope())
            dname.append(d.identifier())
	    member = AST.Variable(node.file(), node.line(), "IDL", "variable", dname, self.getType(dtype), node.constrType())
	    member.comments().extend(comments)
            for c in d.comments():
                member.comments().append(AST.Comment(c.text(), c.file(), c.line()))
	    self.addType(member.name(), Type.Declared("IDL", member.name(), member))
	    self.addDeclaration(member)

    def visitStruct(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        struct = AST.Class(node.file(), node.line(), "IDL", "struct", name)
        self.addDeclaration(struct)
        self.addType(name, Type.Declared("IDL", name, struct))
        for c in node.comments():
            struct.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        self.__scope.append(struct)
        for member in node.members(): member.accept(self)
        self.__scope.pop()
        
    def visitException(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        exc = AST.Class(node.file(), node.line(), "IDL", "exception", name)
        self.addDeclaration(exc)
        self.addType(name, Type.Declared("IDL", name, exc))
        self.__scope.append(exc)
        for c in node.comments():
            exc.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for member in node.members(): member.accept(self)
        self.__scope.pop()
    
    #    def visitCaseLabel(self, node):    return
    def visitUnionCase(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        # if this is an inline constructed type, it is a 'Declared' type
        # and we need to visit the declaration first
        if node.constrType():
            node.caseType().decl().accept(self)
        type = self.__types.internalize(node.caseType())
        declarator = node.declarator()
        if declarator.sizes():
            array = Type.Array("IDL", self.getType(type), declarator.sizes())
            type = type[:-1]
            type.append(type[-1] + string.join(map(lambda s:"["+s+"]",node.sizes()),''))
            self.addType(type, array)
        name = list(self.scope())
        name.append(node.declarator().identifier())
        self.__scope[-1].declarations().append(AST.Operation(node.file(), node.line(), "IDL", "case",
                                                             [], self.getType(type), name, list(name)))
    def visitUnion(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        clas = AST.Class(node.file(), node.line(), "IDL", "union", name)
        self.addDeclaration(clas)
        self.__scope.append(clas)
        self.addType(name, Type.Declared("IDL", name, clas))
        for c in node.comments():
            clas.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for c in node.cases(): c.accept(self)
        self.__scope.pop()
        
    def visitEnumerator(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        enum = AST.Enumerator(node.file(), node.line(), "IDL", name, "")
        self.addType(name, Type.Declared("IDL", name, enum))
        self.__enum.enumerators().append(enum)

    def visitEnum(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        name = list(self.scope())
        name.append(node.identifier())
        self.__enum = AST.Enum(node.file(), node.line(), "IDL", name, [])
        self.addDeclaration(self.__enum)
        self.addType(name, Type.Declared("IDL", name, self.__enum))
        for c in node.comments():
            self.__enum.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for enumerator in node.enumerators(): enumerator.accept(self)
        self.__enum = None
        
    def visitAttribute(self, node):
        if self.__mainfile_only and not node.mainFile(): return
        pre = []
        if node.readonly(): pre.append("readonly")
        type = self.__types.internalize(node.attrType())
	comments = []
	for c in node.comments():
	    comments.append(AST.Comment(c.text(), c.file(), c.line()))
        scopename = list(self.scope())
	for id in node.identifiers():
	    name = scopename + [id]
	    attr = AST.Operation(node.file(), node.line(), "IDL", "attribute",
				 pre, self.getType(type), name, list(name))
	    attr.comments().extend(comments)
	    self.addDeclaration(attr)

    def visitParameter(self, node):
        operation = self.__operation
        pre = []
        if node.direction() == 0: pre.append("in")
        elif node.direction() == 1: pre.append("out")
        else: pre.append("inout")
        post = []
        name = self.__types.internalize(node.paramType())
        operation.parameters().append(AST.Parameter(pre, self.getType(name), post, node.identifier()))
    
    def visitOperation(self, node):
        pre = []
        if node.oneway(): pre.append("oneway")
        returnType = self.__types.internalize(node.returnType())
        name = list(self.scope())
        name.append(node.identifier())
        self.__operation = AST.Operation(node.file(), node.line(), "IDL", "operation", pre, self.getType(returnType), name, list(name))
        for c in node.comments():
            self.__operation.comments().append(AST.Comment(c.text(), c.file(), c.line()))
        for p in node.parameters(): p.accept(self)
        for e in node.raises():
            exception = self.getType(e.scopedName())
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

def usage():
    print \
"""
  -I<path>                             Specify include path to be used by the preprocessor
  -m                                   Unly keep declarations from the main file
  -k                                   Comments after declarations are kept for the back-ends
  -K                                   Comments before declarations are kept for the back-ends"""

def __parseArgs(args):
    global preprocessor_args, mainfile_only

    preprocessor_args = []
    mainfile_only = 0
    try:
        opts,remainder = Util.getopt_spec(args, "I:m:kK")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt

        if o == "-I":
            preprocessor_args.append("-I" + a)
        if o == "-m":
            mainfile_only = 1
        elif o == "-k":
            preprocessor_args.append("-C")
            _omniidl.keepComments(0)
        elif o == "-K":
            preprocessor_args.append("-C")
            _omniidl.keepComments(1)

def parse(file, args, typedict, astdict):
    global preprocessor_args, mainfile_only
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
    ast = ASTTranslator(astdict, types, mainfile_only)
    tree.accept(ast)
    return ast.declarations, types.types
