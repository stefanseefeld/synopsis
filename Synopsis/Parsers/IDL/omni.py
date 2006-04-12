#
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import Type, AST, Util
from Synopsis.SourceFile import *
import idlast, idltype, idlvisitor, idlutil
import _omniidl
import sys, getopt, os, os.path, string, types

sourcefile = None

def strip_filename(filename):
   "This is aliased as strip if -b used and basename set"

   if len(basename) > len(filename): return filename
   if filename[:len(basename)] == basename:
      return filename[len(basename):]
   return filename

class TypeTranslator(idlvisitor.TypeVisitor):
   """maps idltype objects to Synopsis.Type objects in a Type.Dictionary"""

   def __init__(self, types):
      self.types = types
      self.__result = None
      self.__basetypes = {idltype.tk_void:       "void",
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
                          idltype.tk_wchar:      "wchar"}

   def internalize(self, idltype):

      idltype.accept(self)
      return self.__result

   def add(self, name, type):

      self.types[name] = type

   def get(self, name):
      return self.types[name]

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

class ASTTranslator(idlvisitor.AstVisitor):

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

      self.__scope.append(AST.Scope(sourcefile, 0, 'file', []))
      # add an 'Object' Type to the Type Dictionary. Don't declare it in the AST since
      # there is no corresponding declaration
      object = AST.Class(sourcefile, 0, 'interface', ['CORBA', 'Object'])
      self.addType(['CORBA', 'Object'], Type.Declared('IDL', ['CORBA', 'Object'], object))
      for n in node.declarations():
         n.accept(self)
      for d in self.__scope[-1].declarations():
         self.declarations.append(d)

   def visitModule(self, node):

      #if self.__mainfile_only and not node.mainFile(): return
      name = list(self.scope()) + [node.identifier()]
      module = AST.Module(sourcefile, node.line(), 'module', name)
      self.addDeclaration(module)
      self.__scope.append(module)
      self.addType(name, Type.Declared('IDL', name, module))
      if not self.__mainfile_only or node.mainFile(): 
         comments = [c.text() for c in node.comments()]
         if comments:
            module.annotations['comments'] = comments
      for n in node.definitions():
         n.accept(self)
      self.__scope.pop()
        
   def visitInterface(self, node):

      name = list(self.scope()) + [node.identifier()]
      clas = AST.Class(sourcefile, node.line(), 'interface', name)
      self.addDeclaration(clas)
      self.__scope.append(clas)
      self.addType(name, Type.Declared('IDL', name, clas))
      if not self.__mainfile_only or node.mainFile(): 
         comments = [c.text() for c in node.comments()]
         if comments:
            clas.annotations['comments'] = comments
      for i in node.inherits():
         parent = self.getType(i.scopedName())
         clas.parents().append(AST.Inheritance("", parent, []))
      for c in node.contents(): c.accept(self)
      self.__scope.pop()
        
   def visitForward(self, node):

      #if self.__mainfile_only and not node.mainFile(): return
      name = list(self.scope())
      name.append(node.identifier())
      forward = AST.Forward(sourcefile, node.line(), 'interface', name)
      self.addDeclaration(forward)
      self.addType(name, Type.Unknown('IDL', name))
        
   def visitConst(self, node):

      if self.__mainfile_only and not node.mainFile(): return
      name = list(self.scope())
      name.append(node.identifier())
      type = self.__types.internalize(node.constType())
      if node.constType().kind() == idltype.tk_enum:
         value = "::" + idlutil.ccolonName(node.value().scopedName())
      else:
         value = str(node.value())
      const = AST.Const(sourcefile, node.line(), 'const',
                        self.getType(type), name, value)
      self.addDeclaration(const)
      comments = [c.text() for c in node.comments()]
      if comments:
         const.annotations['comments'] = comments
        
   def visitTypedef(self, node):

      #if self.__mainfile_only and not node.mainFile(): return
      # if this is an inline constructed type, it is a 'Declared' type
      # and we need to visit the declaration first
      if node.constrType():
         node.memberType().decl().accept(self)
      type = self.__types.internalize(node.aliasType())
      comments = [c.text() for c in node.comments()]
      for d in node.declarators():
         # reinit the type for this declarator, as each declarator of
         # a single typedef declaration can have a different type. *sigh*
         dtype = type
         if d.sizes():
            array = Type.Array('IDL', self.getType(type), map(lambda s:str(s), d.sizes()))
            dtype = map(None, type[:-1])
            dtype.append(type[-1] + string.join(map(lambda s:"["+ str(s) +"]", d.sizes()),''))
            self.addType(dtype, array)
         dname = list(self.scope())
         dname.append(d.identifier())
         typedef = AST.Typedef(sourcefile, node.line(), 'typedef', dname, self.getType(dtype), node.constrType())
         d_comments = comments + [c.text() for c in d.comments()]
         if d_comments:
            typedef.annotations['comments'] = d_comments
         self.addType(typedef.name(), Type.Declared('IDL', typedef.name(), typedef))
         self.addDeclaration(typedef)

   def visitMember(self, node):

      if self.__mainfile_only and not node.mainFile(): return
      # if this is an inline constructed type, it is a 'Declared' type
      # and we need to visit the declaration first
      if node.constrType():
         node.memberType().decl().accept(self)
      type = self.__types.internalize(node.memberType())
      comments = [c.text() for c in node.comments()]
      for d in node.declarators():
         # reinit the type for this declarator, as each declarator of
         # a single typedef declaration can have a different type. *sigh*
         dtype = type
         if d.sizes():
            array = Type.Array('IDL', self.getType(type), [str(s) for s in node.sizes()])
            dtype = type[:-1]
            dtype.append(type[-1] + string.join(map(lambda s:"["+s+"]", d.sizes()),''))
            self.addType(dtype, array)
         dname = list(self.scope())
         dname.append(d.identifier())
         member = AST.Variable(sourcefile, node.line(), 'variable', dname, self.getType(dtype), node.constrType())
         d_comments = comments + [c.text() for c in d.comments()]
         if d_comments:
            member.annotations['comments'] = d_comments
         self.addType(member.name(), Type.Declared('IDL', member.name(), member))
         self.addDeclaration(member)

   def visitStruct(self, node):

      name = list(self.scope()) + [node.identifier()]
      if self.__mainfile_only and not node.mainFile():
         forward = AST.Forward(sourcefile, node.line(), 'struct', name)
         self.addDeclaration(forward)
         self.addType(name, Type.Declared('IDL', name, forward))
         return
      struct = AST.Class(sourcefile, node.line(), 'struct', name)
      self.addDeclaration(struct)
      self.addType(name, Type.Declared('IDL', name, struct))
      comments = [c.text() for c in node.comments()]
      if comments:
         struct.annotations['comments'] = comments
      self.__scope.append(struct)
      for member in node.members(): member.accept(self)
      self.__scope.pop()
        
   def visitException(self, node):

      name = list(self.scope()) + [node.identifier()]
      if self.__mainfile_only and not node.mainFile():
         forward = AST.Forward(sourcefile, node.line(), 'exception', name)
         self.addDeclaration(forward)
         self.addType(name, Type.Declared('IDL', name, forward))
         return
      exc = AST.Class(sourcefile, node.line(), 'exception', name)
      self.addDeclaration(exc)
      self.addType(name, Type.Declared('IDL', name, exc))
      self.__scope.append(exc)
      comments = [c.text() for c in node.comments()]
      if comments:
         exc.annotations['comments'] = comments
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
         array = Type.Array('IDL', self.getType(type), [str(s) for s in declarator.sizes()])
         type = type[:-1]
         type.append(type[-1] + string.join(map(lambda s:"["+s+"]",node.sizes()),''))
         self.addType(type, array)
      name = list(self.scope())
      name.append(node.declarator().identifier())
      self.__scope[-1].declarations().append(
         AST.Operation(sourcefile, node.line(), 'case',
			  [], self.getType(type), name, name[-1]))

   def visitUnion(self, node):

      name = list(self.scope()) + [node.identifier()]
      if self.__mainfile_only and not node.mainFile():
         forward = AST.Forward(sourcefile, node.line(), 'union', name)
         self.addDeclaration(forward)
         self.addType(name, Type.Declared('IDL', name, forward))
         return
      clas = AST.Class(sourcefile, node.line(), 'union', name)
      self.addDeclaration(clas)
      self.__scope.append(clas)
      self.addType(name, Type.Declared('IDL', name, clas))
      comments = [c.text() for c in node.comments()]
      if comments:
         clas.annotations['comments'] = comments
      for c in node.cases(): c.accept(self)
      self.__scope.pop()
        
   def visitEnumerator(self, node):

      if self.__mainfile_only and not node.mainFile(): return
      name = list(self.scope())
      name.append(node.identifier())
      enum = AST.Enumerator(sourcefile, node.line(), name, "")
      self.addType(name, Type.Declared('IDL', name, enum))
      self.__enum.enumerators().append(enum)

   def visitEnum(self, node):

      name = list(self.scope()) + [node.identifier()]
      if self.__mainfile_only and not node.mainFile():
         forward = AST.Forward(sourcefile, node.line(), 'enum', name)
         self.addDeclaration(forward)
         self.addType(name, Type.Declared('IDL', name, forward))
         return
      self.__enum = AST.Enum(sourcefile, node.line(), name, [])
      self.addDeclaration(self.__enum)
      self.addType(name, Type.Declared('IDL', name, self.__enum))
      comments = [c.text() for c in node.comments()]
      if comments:
         self.__enum.annotations['comments'] = comments
      for enumerator in node.enumerators(): enumerator.accept(self)
      self.__enum = None
        
   def visitAttribute(self, node):

      scopename = list(self.scope())
      if self.__mainfile_only and not node.mainFile(): return
      # Add real Operation objects
      pre = []
      if node.readonly(): pre.append("readonly")
      type = self.__types.internalize(node.attrType())
      comments = [c.text() for c in node.comments()]
      for id in node.identifiers():
         name = scopename + [id]
         attr = AST.Operation(sourcefile, node.line(), 'attribute',
                              pre, self.getType(type), [], name, name[-1])
         if comments:
            attr.annotations['comments'] = comments
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
      self.__operation = AST.Operation(sourcefile, node.line(), 'operation', pre, self.getType(returnType), [], name, name[-1])
      comments = [c.text() for c in node.comments()]
      if comments:
         self.__operation.annotations['comments'] = comments
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

def parse(ast, cppfile, src, primary_file_only,
          base_path, verbose, debug):
   global basename, strip, sourcefile

   if base_path:
      basename = base_path

   _omniidl.keepComments(1)
   _omniidl.noForwardWarning()
   tree = _omniidl.compile(open(cppfile, 'r+'))
   if tree == None:
      sys.stderr.write("omni: Error parsing %s\n"%cppfile)
      sys.exit(1)

   sourcefile = SourceFile(strip_filename(src), src, 'IDL')
   sourcefile.annotations['primary'] = True
   new_ast = AST.AST()
   new_ast.files()[sourcefile.name] = sourcefile
   type_trans = TypeTranslator(new_ast.types())
   ast_trans = ASTTranslator(new_ast.declarations(), type_trans, primary_file_only)
   tree.accept(ast_trans)
   sourcefile.declarations[:] = new_ast.declarations()
   ast.merge(new_ast)
   _omniidl.clear()
   return ast
