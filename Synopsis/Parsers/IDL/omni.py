#
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import IR, ASG
from Synopsis.QualifiedName import QualifiedCxxName as QName
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
   """maps idltype objects to ASG.Type objects in a ASG.Dictionary"""

   def __init__(self, types):
      self.types = types
      self.__result = None
      self.__basetypes = {idltype.tk_void:       QName(('void',)),
                          idltype.tk_short:      QName(('short',)),
                          idltype.tk_long:       QName(('long',)),
                          idltype.tk_ushort:     QName(('unsigned short',)),
                          idltype.tk_ulong:      QName(('unsigned long',)),
                          idltype.tk_float:      QName(('float',)),
                          idltype.tk_double:     QName(('double',)),
                          idltype.tk_boolean:    QName(('boolean',)),
                          idltype.tk_char:       QName(('char',)),
                          idltype.tk_octet:      QName(('octet',)),
                          idltype.tk_any:        QName(('any',)),
                          idltype.tk_TypeCode:   QName(('CORBA','TypeCode',)),
                          idltype.tk_Principal:  QName(('CORBA','Principal',)),
                          idltype.tk_longlong:   QName(('long long',)),
                          idltype.tk_ulonglong:  QName(('unsigned long long',)),
                          idltype.tk_longdouble: QName(('long double',)),
                          idltype.tk_wchar:      QName(('wchar',))}

   def internalize(self, idltype):

      idltype.accept(self)
      return self.__result

   def has_key(self, name): return self.types.has_key(name)

   def add(self, name, type):
      self.types[name] = type

   def get(self, name):
      return self.types[name]

   def visitBaseType(self, idltype):

      type = ASG.BaseType('IDL', self.__basetypes[idltype.kind()])
      self.types[type.name] = type
      self.__result = type.name

   def visitStringType(self, idltype):

      # FIXME: Should we create a Parametrized with the appropriate bound parameters ?
      if idltype.bound() == 0:
         qname = QName(('string',))
      else:
         qname = QName(('string<%s>'%idltype.bound(),))
      if qname not in self.types:
         self.types[qname] = ASG.BaseType('IDL', qname)
      self.__result = qname

   def visitWStringType(self, idltype):

      # FIXME: Should we create a Parametrized with the appropriate bound parameters ?
      if idltype.bound() == 0:
         qname = QName(('wstring',))
      else:
         qname = QName(('wstring<%s>'%idltype.bound(),))
      if qname not in self.types:
         self.types[qname] = ASG.BaseType('IDL', qname)
      self.__result = qname

   def visitSequenceType(self, idltype):

      qname = QName(('sequence',))
      if not self.types.has_key(qname):
         self.types[qname] = ASG.BaseType("IDL", qname)
      idltype.seqType().accept(self)
      ptype = self.types[self.__result]
      type = ASG.Parametrized("IDL", self.types[qname], [ptype])
      qname  = QName(('sequence<%s>'%str(ptype.name),))
      self.types[qname] = type
      self.__result = qname
        
   def visitDeclaredType(self, idltype):

      self.__result = QName(idltype.decl().scopedName())

class ASGTranslator(idlvisitor.AstVisitor):

   def __init__(self, declarations, types, primary_file_only):

      self.declarations = declarations
      self.primary_file_only = primary_file_only
      self.types = types
      self.__scope = []
      self.__operation = None
      self.__enum = None
        
   def scope(self): return self.__scope[-1].name

   def add_declaration(self, declaration):
      self.__scope[-1].declarations.append(declaration)

   def addType(self, name, type):

      if self.types.has_key(name):
         if isinstance(self.types.get(name), ASG.UnknownType):
            self.types.add(name, type)
         else:
            pass
         return
      self.types.add(name, type)

   def getType(self, name): return self.types.get(QName(name))
   def visitAST(self, node):

      self.__scope.append(ASG.Scope(sourcefile, 0, 'file', QName()))
      # add an 'Object' Type to the Type Dictionary. Don't declare it in the ASG since
      # there is no corresponding declaration
      qname = QName(('CORBA', 'Object'))
      object = ASG.Class(sourcefile, 0, 'interface', qname)
      self.addType(qname, ASG.Declared('IDL', qname, object))
      for n in node.declarations():
         n.accept(self)
      for d in self.__scope[-1].declarations:
         self.declarations.append(d)

   def visitModule(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      module = ASG.Module(sourcefile, node.line(), 'module', qname)
      if visible:
         self.add_declaration(module)
      self.__scope.append(module)
      self.addType(qname, ASG.Declared('IDL', qname, module))
      if not self.primary_file_only or node.mainFile(): 
         comments = [c.text() for c in node.comments()]
         if comments:
            module.annotations['comments'] = comments
      for n in node.definitions():
         n.accept(self)
      self.__scope.pop()
        
   def visitInterface(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      class_ = ASG.Class(sourcefile, node.line(), 'interface', qname)
      if visible:
         self.add_declaration(class_)
      self.__scope.append(class_)
      self.addType(qname, ASG.Declared('IDL', qname, class_))
      if not self.primary_file_only or node.mainFile(): 
         comments = [c.text() for c in node.comments()]
         if comments:
            class_.annotations['comments'] = comments
      for i in node.inherits():
         parent = self.getType(i.scopedName())
         class_.parents.append(ASG.Inheritance("", parent, []))
      for c in node.contents(): c.accept(self)
      self.__scope.pop()
        
   def visitForward(self, node):

      visible = node.mainFile() or not self.primary_file_only
      name = list(self.scope())
      qname = QName(name + [node.identifier()])
      forward = ASG.Forward(sourcefile, node.line(), 'interface', qname)
      if visible:
         self.add_declaration(forward)
      self.addType(qname, ASG.UnknownType('IDL', qname))
        
   def visitConst(self, node):

      visible = node.mainFile() or not self.primary_file_only
      name = list(self.scope())
      qname = QName(name + [node.identifier()])
      type = self.types.internalize(node.constType())
      if node.constType().kind() == idltype.tk_enum:
         value = "::" + idlutil.ccolonName(node.value().scopedName())
      else:
         value = str(node.value())
      const = ASG.Const(sourcefile, node.line(), 'const',
                        self.getType(type), qname, value)
      if visible:
         self.add_declaration(const)
      comments = [c.text() for c in node.comments()]
      if comments:
         const.annotations['comments'] = comments
        
   def visitTypedef(self, node):

      visible = node.mainFile() or not self.primary_file_only
      # if this is an inline constructed type, it is a 'Declared' type
      # and we need to visit the declaration first
      if node.constrType():
         node.aliasType().decl().accept(self)
      type = self.types.internalize(node.aliasType())
      comments = [c.text() for c in node.comments()]
      for d in node.declarators():
         # reinit the type for this declarator, as each declarator of
         # a single typedef declaration can have a different type. *sigh*
         dtype = type
         if d.sizes():
            array = ASG.Array('IDL', self.getType(type), [str(s) for s in d.sizes()])
            dtype = map(None, type[:-1])
            dtype.append(type[-1] + string.join(map(lambda s:"["+ str(s) +"]", d.sizes()),''))
            self.addType(QName(dtype), array)
         name = list(self.scope())
         qname = QName(name + [d.identifier()])
         typedef = ASG.Typedef(sourcefile, node.line(), 'typedef', qname, self.getType(dtype), node.constrType())
         d_comments = comments + [c.text() for c in d.comments()]
         if d_comments:
            typedef.annotations['comments'] = d_comments
         self.addType(qname, ASG.Declared('IDL', qname, typedef))
         if visible:
            self.add_declaration(typedef)

   def visitMember(self, node):

      visible = node.mainFile() or not self.primary_file_only
      # if this is an inline constructed type, it is a 'Declared' type
      # and we need to visit the declaration first
      if node.constrType():
         node.memberType().decl().accept(self)
      type = self.types.internalize(node.memberType())
      comments = [c.text() for c in node.comments()]
      for d in node.declarators():
         # reinit the type for this declarator, as each declarator of
         # a single typedef declaration can have a different type. *sigh*
         dtype = type
         if d.sizes():
            array = ASG.Array('IDL', self.getType(type), [str(s) for s in node.sizes()])
            dtype = type[:-1]
            dtype.append(type[-1] + string.join(map(lambda s:"["+s+"]", d.sizes()),''))
            self.addType(dtype, array)
         qname = QName(list(self.scope()) + [d.identifier()])
         member = ASG.Variable(sourcefile, node.line(), 'variable', qname, self.getType(dtype), node.constrType())
         d_comments = comments + [c.text() for c in d.comments()]
         if d_comments:
            member.annotations['comments'] = d_comments
         self.addType(qname, ASG.Declared('IDL', qname, member))
         if visible:
            self.add_declaration(member)

   def visitStruct(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      if self.primary_file_only and not node.mainFile():
         forward = ASG.Forward(sourcefile, node.line(), 'struct', qname)
         if visible:
            self.add_declaration(forward)
         self.addType(qname, ASG.Declared('IDL', qname, forward))
         return
      struct = ASG.Class(sourcefile, node.line(), 'struct', qname)
      if visible:
         self.add_declaration(struct)
      self.addType(qname, ASG.Declared('IDL', qname, struct))
      comments = [c.text() for c in node.comments()]
      if comments:
         struct.annotations['comments'] = comments
      self.__scope.append(struct)
      for member in node.members(): member.accept(self)
      self.__scope.pop()
        
   def visitException(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      if self.primary_file_only and not node.mainFile():
         forward = ASG.Forward(sourcefile, node.line(), 'exception', qname)
         if visible:
            self.add_declaration(forward)
         self.addType(qname, ASG.Declared('IDL', qname, forward))
         return
      exc = ASG.Class(sourcefile, node.line(), 'exception', qname)
      if visible:
         self.add_declaration(exc)
      self.addType(qname, ASG.Declared('IDL', qname, exc))
      self.__scope.append(exc)
      comments = [c.text() for c in node.comments()]
      if comments:
         exc.annotations['comments'] = comments
      for member in node.members(): member.accept(self)
      self.__scope.pop()
    
    #    def visitCaseLabel(self, node):    return

   def visitUnionCase(self, node):

      # if this is an inline constructed type, it is a 'Declared' type
      # and we need to visit the declaration first
      if node.constrType():
         node.caseType().decl().accept(self)
      type = self.types.internalize(node.caseType())
      declarator = node.declarator()
      if declarator.sizes():
         array = ASG.Array('IDL', self.getType(type), [str(s) for s in declarator.sizes()])
         type = type[:-1]
         type.append(type[-1] + string.join(map(lambda s:"["+s+"]",node.sizes()),''))
         self.addType(type, array)
      qname = QName(list(self.scope()) + [node.declarator().identifier()])
      self.__scope[-1].declarations.append(
         ASG.Operation(sourcefile, node.line(), 'case',
			  [], self.getType(type), [], qname, qname[-1]))

   def visitUnion(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      if self.primary_file_only and not node.mainFile():
         forward = ASG.Forward(sourcefile, node.line(), 'union', qname)
         if visible:
            self.add_declaration(forward)
         self.addType(qname, ASG.Declared('IDL', qname, forward))
         return
      class_ = ASG.Class(sourcefile, node.line(), 'union', qname)
      self.add_declaration(class_)
      self.__scope.append(class_)
      self.addType(qname, ASG.Declared('IDL', qname, class_))
      comments = [c.text() for c in node.comments()]
      if comments:
         class_.annotations['comments'] = comments
      for c in node.cases(): c.accept(self)
      self.__scope.pop()
        
   def visitEnumerator(self, node):

      qname = QName(list(self.scope()) + [node.identifier()])
      enum = ASG.Enumerator(sourcefile, node.line(), qname, '')
      self.addType(qname, ASG.Declared('IDL', qname, enum))
      self.__enum.enumerators.append(enum)

   def visitEnum(self, node):

      visible = node.mainFile() or not self.primary_file_only
      qname = QName(list(self.scope()) + [node.identifier()])
      if self.primary_file_only and not node.mainFile():
         forward = ASG.Forward(sourcefile, node.line(), 'enum', qname)
         if visible:
            self.add_declaration(forward)
         self.addType(qname, ASG.Declared('IDL', qname, forward))
         return
      self.__enum = ASG.Enum(sourcefile, node.line(), qname, [])
      if visible:
         self.add_declaration(self.__enum)
      self.addType(qname, ASG.Declared('IDL', qname, self.__enum))
      comments = [c.text() for c in node.comments()]
      if comments:
         self.__enum.annotations['comments'] = comments
      for enumerator in node.enumerators(): enumerator.accept(self)
      self.__enum = None
        
   def visitAttribute(self, node):

      visible = node.mainFile() or not self.primary_file_only
      scopename = list(self.scope())
      if self.primary_file_only and not node.mainFile(): return
      # Add real Operation objects
      pre = []
      if node.readonly(): pre.append("readonly")
      type = self.types.internalize(node.attrType())
      comments = [c.text() for c in node.comments()]
      for id in node.identifiers():
         qname = QName(scopename + [id])
         attr = ASG.Operation(sourcefile, node.line(), 'attribute',
                              pre, self.getType(type), [], qname, qname[-1])
         if comments:
            attr.annotations['comments'] = comments
         if visible:
            self.add_declaration(attr)

   def visitParameter(self, node):

      operation = self.__operation
      pre = []
      if node.direction() == 0: pre.append("in")
      elif node.direction() == 1: pre.append("out")
      else: pre.append("inout")
      post = []
      name = self.types.internalize(node.paramType())
      operation.parameters.append(ASG.Parameter(pre, self.getType(name), post, node.identifier()))
    
   def visitOperation(self, node):

      visible = node.mainFile() or not self.primary_file_only
      pre = []
      if node.oneway(): pre.append("oneway")
      return_type = self.types.internalize(node.returnType())
      qname = QName(list(self.scope()) + [node.identifier()])
      self.__operation = ASG.Operation(sourcefile, node.line(), 'operation', pre, self.getType(return_type), [], qname, qname[-1])
      comments = [c.text() for c in node.comments()]
      if comments:
         self.__operation.annotations['comments'] = comments
      for p in node.parameters(): p.accept(self)
      for e in node.raises():
         exception = self.getType(e.scopedName())
         self.__operation.exceptions.append(exception)
            
      if visible:
         self.add_declaration(self.__operation)
      self.__operation = None
        
#    def visitNative(self, node):       return
#    def visitStateMember(self, node):  return
#    def visitFactory(self, node):      return
#    def visitValueForward(self, node): return
#    def visitValueBox(self, node):     return
#    def visitValueAbs(self, node):     return
#    def visitValue(self, node):        return

def parse(ir, cppfile, src, primary_file_only,
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
   new_ir = IR.IR()
   new_ir.files[sourcefile.name] = sourcefile
   type_trans = TypeTranslator(new_ir.types)
   ast_trans = ASGTranslator(new_ir.declarations, type_trans, primary_file_only)
   tree.accept(ast_trans)
   sourcefile.declarations[:] = new_ir.declarations
   ir.merge(new_ir)
   _omniidl.clear()
   return ir
