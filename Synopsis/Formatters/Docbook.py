#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.2 XML output"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG, Type, Util

import sys, getopt, os, os.path, re

languages = {
    'IDL': 'idl',
    'C++': 'cxx',
    'Python': 'python'
    }

class Formatter(Processor, Type.Visitor, ASG.Visitor):
   """
   The type visitors should generate names relative to the current scope.
   The generated references however are fully scoped names
   """
   
   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      self.ir = self.merge_input(ir)

      self.__os = open(self.output, 'w')
      self.__scope = ()
      self.__scopestack = []
      self.__indent = 0

      for d in self.ir.declarations:
         d.accept(self)

      self.__os.close()
      
      return self.ir

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

   def start_element(self, type, **params):
      """Write the start of an element, ending with a newline"""

      param_text = ''
      if __params: param_text = ' ' + ''.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
      self.write('<' + type + param_text + '>')
      self.__indent = self.__indent + 2
      self.write('\n')

   def end_element(self, type):
      """Write the end of an element, starting with a newline"""

      self.__indent = self.__indent - 2
      self.write('\n</' + type + '>')

   def write_element(self, type, body, **params):
      """Write a single element on one line (though body may contain
      newlines)"""

      param_text = ''
      if params: param_text = ' ' + ''.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
      self.write('<' + type + param_text + '>')
      self.__indent = self.__indent + 2
      self.write(body)
      self.__indent = self.__indent - 2
      self.write('</' + type + '>')

   def element(self, type, body, **params):
      """Return but do not write the text for an element on one line"""

      param_text = ''
      if params: param_text = ' ' + ''.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
      return '<%s%s>%s</%s>'%(type, param_text, body, type)

   def reference(self, ref, label):
      """reference takes two strings, a reference (used to look up the symbol and generated the reference),
      and the label (used to actually write it)"""

      location = self.__toc.lookup(ref)
      if location != '': return href('#' + location, label)
      else: return span('type', str(label))

   def label(self, ref):

      location = self.__toc.lookup(Util.ccolonName(ref))
      ref = Util.ccolonName(ref, self.scope())
      if location != '': return name('"' + location + '"', ref)
      else: return ref

   def type_label(self): return self.__type_label

   #################### Type Visitor ##########################################

   def visit_base_type(self, type):

      self.__type_ref = Util.ccolonName(type.name)
      self.__type_label = Util.ccolonName(type.name)

   def visit_unknown(self, type):

      self.__type_ref = Util.ccolonName(type.name)
      self.__type_label = Util.ccolonName(type.name, self.scope())
        
   def visit_declared(self, type):

      self.__type_label = Util.ccolonName(type.name, self.scope())
      self.__type_ref = Util.ccolonName(type.name)

   def visit_modifier(self, type):

      type.alias.accept(self)
      self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
      self.__type_label = ''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod)

   def visit_parametrized(self, type):

      type.template.accept(self)
      type_label = self.__type_label + '&lt;'
      parameters_label = []
      for p in type.parameters:
         p.accept(self)
         parameters_label.append(self.__type_label)
      self.__type_label = type_label + ', '.join(parameters_label) + '&gt;'

   def visit_function_type(self, type):

      # TODO: this needs to be implemented
      self.__type_ref = 'function_type'
      self.__type_label = 'function_type'

   def process_doc(self, doc):

      text = doc.replace('\n\n', '</para><para>')
      self.write(self.element("para", text)+'\n')

   #################### ASG Visitor ###########################################

   def visit_declarator(self, node):

      self.__declarator = node.name
      for i in node.sizes:
         self.__declarator[-1] = self.__declarator[-1] + '[%d]'%i

   def visit_typedef(self, typedef):

      print "sorry, <typedef> not implemented"

   def visit_variable(self, variable):

      self.start_element("fieldsynopsis")
      variable.vtype.accept(self)
      self.element("type", self.type_label())
      self.element("varname", variable.name[-1])
      self.end_element("fieldsynopsis")

   def visit_const(self, const):

      print "sorry, <const> not implemented"

   def visit_module(self, module):

      self.start_element("section")
      self.write_element("title", module.type+" "+Util.ccolonName(module.name))
      self.write("\n")
      self.process_doc(module.annotations.get('doc', ''))
      self.push_scope(module.name)
      for declaration in module.declarations: declaration.accept(self)
      self.pop_scope()
      self.end_element("section")

   def visit_class(self, class_):

      self.start_element("classsynopsis", Class=class_.type, language=languages[class_.language])
      classname = self.element("classname", Util.ccolonName(class_.name))
      self.write_element("ooclass", classname)
      self.start_element("classsynopsisinfo")
      for p in class_.parents:
         p.accept(self)
      self.push_scope(class_.name)
      if class_.annotations.has_key('doc'):
         self.start_element("textobject")
         self.process_doc(class_.annotations['doc'])
         self.end_element("textobject")
      self.end_element("classsynopsisinfo")
      classes = []
      for d in class_.declarations:
         # Store classes for later
         if isinstance(d, ASG.Class):
            classes.append(d)
         else:
            d.accept(self)
      self.pop_scope()
      self.end_element("classsynopsis")
      # Classes can't be nested (in docbook 4.2), so do them after
      for c in classes:
         c.accept(self)

   def visit_inheritance(self, inheritance):

      map(lambda a, this=self: this.element("modifier", a), inheritance.attributes())
      self.element("classname", Util.ccolonName(inheritance.parent.name, self.scope()))

   def visit_parameter(self, parameter):

      self.start_element("methodparam")
      map(lambda m, this=self: this.write_element("modifier", m), parameter.premodifier)
      parameter.type.accept(self)
      self.write_element("type", self.type_label())
      self.write_element("parameter", parameter.name)
      map(lambda m, this=self: this.write_element("modifier", m), parameter.postmodifier)
      self.end_element("methodparam")

   def visit_function(self, function):

      print "sorry, <function> not implemented"

   def visit_operation(self, operation):

      if operation.language == "IDL" and operation.type == "attribute":
         self.start_element("fieldsynopsis")
         map(lambda m, this=self: this.element("modifier", m), operation.premodifiers())
         self.write_element("modifier", "attribute")
         operation.return_type.accept(self)
         self.write_element("type", self.type_label())
         self.write_element("varname", operation.real_name)
         self.end_element("fieldsynopsis")
      else:
         self.start_element("methodsynopsis")
         if operation.language != "Python":
            ret = operation.return_type
            if ret:
               ret.accept(self)
               self.write_element("type", self.type_label())
         else:
            self.write_element("modifier", "def")
         self.write_element("methodname", Util.ccolonName(operation.real_name, self.scope()))
         for parameter in operation.parameters: parameter.accept(self)
         map(lambda e, this=self: this.element("exceptionname", e), operation.exceptions)
         self.end_element("methodsynopsis")

   def visit_enumerator(self, enumerator):
      print "sorry, <enumerator> not implemented"

   def visit_enum(self, enum):
      print "sorry, <enum> not implemented"

