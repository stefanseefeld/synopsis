#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.2 XML output"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util

import sys, getopt, os, os.path, string, re

languages = {
    'IDL': 'idl',
    'C++': 'cxx',
    'Python': 'python'
    }

class Formatter(Processor, Type.Visitor, AST.Visitor):
   """
   The type visitors should generate names relative to the current scope.
   The generated references however are fully scoped names
   """
   
   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      self.__os = open(self.output, 'w')
      self.__scope = ()
      self.__scopestack = []
      self.__indent = 0

      for d in self.ast.declarations():
         d.accept(self)

      self.__os.close()
      
      return self.ast

   def scope(self): return self.__scope
   def push_scope(self, newscope):

      self.__scopestack.append(self.__scope)
      self.__scope = newscope

   def pop_scope(self):

      self.__scope = self.__scopestack[-1]
      del self.__scopestack[-1]

   def write(self, text):
      """Write some text to the output stream, replacing \n's with \n's and
      indents."""

      indent = ' ' * self.__indent
      self.__os.write(text.replace('\n', '\n'+indent))

   def start_entity(self, __type, **__params):
      """Write the start of an entity, ending with a newline"""

      param_text = ""
      if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
      self.write("<" + __type + param_text + ">")
      self.__indent = self.__indent + 2
      self.write("\n")

   def end_entity(self, type):
      """Write the end of an entity, starting with a newline"""

      self.__indent = self.__indent - 2
      self.write("\n</" + type + ">")

   def write_entity(self, __type, __body, **__params):
      """Write a single entity on one line (though body may contain
      newlines)"""

      param_text = ""
      if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
      self.write("<" + __type + param_text + ">")
      self.__indent = self.__indent + 2
      self.write(__body)
      self.__indent = self.__indent - 2
      self.write("</" + __type + ">")

   def entity(self, __type, __body, **__params):
      """Return but do not write the text for an entity on one line"""

      param_text = ""
      if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
      return "<%s%s>%s</%s>"%(__type, param_text, __body, __type)

   def reference(self, ref, label):
      """reference takes two strings, a reference (used to look up the symbol and generated the reference),
      and the label (used to actually write it)"""

      location = self.__toc.lookup(ref)
      if location != "": return href("#" + location, label)
      else: return span("type", str(label))

   def label(self, ref):

      location = self.__toc.lookup(Util.ccolonName(ref))
      ref = Util.ccolonName(ref, self.scope())
      if location != "": return name("\"" + location + "\"", ref)
      else: return ref

   def type_label(self): return self.__type_label

   #################### Type Visitor ##########################################

   def visitBaseType(self, type):

      self.__type_ref = Util.ccolonName(type.name())
      self.__type_label = Util.ccolonName(type.name())

   def visitUnknown(self, type):

      self.__type_ref = Util.ccolonName(type.name())
      self.__type_label = Util.ccolonName(type.name(), self.scope())
        
   def visitDeclared(self, type):

      self.__type_label = Util.ccolonName(type.name(), self.scope())
      self.__type_ref = Util.ccolonName(type.name())

   def visitModifier(self, type):

      type.alias().accept(self)
      self.__type_ref = string.join(type.premod()) + " " + self.__type_ref + " " + string.join(type.postmod())
      self.__type_label = string.join(type.premod()) + " " + self.__type_label + " " + string.join(type.postmod())

   def visitParametrized(self, type):

      type.template().accept(self)
      type_label = self.__type_label + "&lt;"
      parameters_label = []
      for p in type.parameters():
         p.accept(self)
         parameters_label.append(self.__type_label)
      self.__type_label = type_label + string.join(parameters_label, ", ") + "&gt;"

   def visitFunctionType(self, type):

      # TODO: this needs to be implemented
      self.__type_ref = 'function_type'
      self.__type_label = 'function_type'

   def process_doc(self, doc):

      text = doc.replace('\n\n', '</para><para>')
      self.write(self.entity("para", text)+'\n')

   #################### AST Visitor ###########################################

   def visitDeclarator(self, node):

      self.__declarator = node.name()
      for i in node.sizes():
         self.__declarator[-1] = self.__declarator[-1] + "[" + str(i) + "]"

   def visitTypedef(self, typedef):

      print "sorry, <typedef> not implemented"

   def visitVariable(self, variable):

      self.start_entity("fieldsynopsis")
      variable.vtype().accept(self)
      self.entity("type", self.type_label())
      self.entity("varname", variable.name()[-1])
      self.end_entity("fieldsynopsis")

   def visitConst(self, const):

      print "sorry, <const> not implemented"

   def visitModule(self, module):

      self.start_entity("section")
      self.write_entity("title", module.type()+" "+Util.ccolonName(module.name()))
      self.write("\n")
      self.process_doc(module.annotations.get('doc', '')
      self.push_scope(module.name())
      for declaration in module.declarations(): declaration.accept(self)
      self.pop_scope()
      self.end_entity("section")

   def visitClass(self, clas):

      self.start_entity("classsynopsis", Class=clas.type(), language=languages[clas.language()])
      classname = self.entity("classname", Util.ccolonName(clas.name()))
      self.write_entity("ooclass", classname)
      self.start_entity("classsynopsisinfo")
      if len(clas.parents()):
         for parent in clas.parents(): parent.accept(self)
      self.push_scope(clas.name())
      if clas.annotations.has_key('doc'):
         self.start_entity("textobject")
         self.process_doc(clas.annotations['doc'])
         self.end_entity("textobject")
      self.end_entity("classsynopsisinfo")
      classes = []
      for declaration in clas.declarations():
         # Store classes for later
         if isinstance(declaration, AST.Class):
            classes.append(declaration)
         else:
            declaration.accept(self)
      self.pop_scope()
      self.end_entity("classsynopsis")
      # Classes can't be nested (in docbook 4.2), so do them after
      for clas in classes: clas.accept(self)

   def visitInheritance(self, inheritance):

      map(lambda a, this=self: this.entity("modifier", a), inheritance.attributes())
      self.entity("classname", Util.ccolonName(inheritance.parent().name(), self.scope()))

   def visitParameter(self, parameter):

      self.start_entity("methodparam")
      map(lambda m, this=self: this.write_entity("modifier", m), parameter.premodifier())
      parameter.type().accept(self)
      self.write_entity("type", self.type_label())
      self.write_entity("parameter", parameter.identifier())
      map(lambda m, this=self: this.write_entity("modifier", m), parameter.postmodifier())
      self.end_entity("methodparam")

   def visitFunction(self, function):

      print "sorry, <function> not implemented"

   def visitOperation(self, operation):

      if operation.language() == "IDL" and operation.type() == "attribute":
         self.start_entity("fieldsynopsis")
         map(lambda m, this=self: this.entity("modifier", m), operation.premodifiers())
         self.write_entity("modifier", "attribute")
         operation.returnType().accept(self)
         self.write_entity("type", self.type_label())
         self.write_entity("varname", operation.realname())
         self.end_entity("fieldsynopsis")
      else:
         self.start_entity("methodsynopsis")
         if operation.language() != "Python":
            ret = operation.returnType()
            if ret:
               ret.accept(self)
               self.write_entity("type", self.type_label())
         else:
            self.write_entity("modifier", "def")
         self.write_entity("methodname", Util.ccolonName(operation.realname(), self.scope()))
         for parameter in operation.parameters(): parameter.accept(self)
         map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
         self.end_entity("methodsynopsis")

   def visitEnumerator(self, enumerator):
      print "sorry, <enumerator> not implemented"
   def visitEnum(self, enum):
      print "sorry, <enum> not implemented"

