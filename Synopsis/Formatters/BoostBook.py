#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a BoostBook formatter"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG, Util

import sys, getopt, os, os.path

class Formatter(Processor, ASG.Visitor):
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
      self.__type_label = ''
      
      for d in ir.declarations:
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

      location = self.__toc.lookup(str(ref))
      ref = Util.ccolonName(ref, self.scope())
      if location != "": return name("\"" + location + "\"", ref)
      else: return ref

   def type_label(self): return self.__type_label
   #################### Type Visitor ##########################################

   def visit_base_type(self, type):

      self.__type_ref = Util.ccolonName(type.name)
      self.__type_label = Util.ccolonName(type.name)
        
   def visit_unknown_type(self, type):

      self.__type_ref = Util.ccolonName(type.name)
      self.__type_label = Util.ccolonName(type.name, self.scope())

   def visit_declared_type(self, type):

      self.__type_label = Util.ccolonName(type.name, self.scope())
      self.__type_ref = Util.ccolonName(type.name)

   def visit_modifier_type(self, type):

      type.alias.accept(self)
      self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
      self.__type_label = ''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod)

   def visit_parametrized(self, type):

      type.template.accept(self)
      type_label = self.__type_label + "&lt;"
      parameters_label = []
      for p in type.parameters:
         p.accept(self)
         parameters_label.append(self.__type_label)
      self.__type_label = type_label + ', '.join(parameters_label) + '&gt;'

   def format_type(self, type):

      type.accept(self)
      return self.__type_label

   def visit_function_type(self, type):

      # TODO: this needs to be implemented
      self.__type_ref = 'function_type'
      self.__type_label = 'function_type'

   def process_doc(self, doc):

      text = doc.replace('\n\n', '</para><para>')
      self.write(self.entity("para", text)+'\n')

   #################### ASG Visitor ###########################################
         
   def visit_declarator(self, node):
      self.__declarator = node.name
      for i in node.sizes:
         self.__declarator[-1] = self.__declarator[-1] + '[%d]'%i

   def visit_typedef(self, typedef):

      self.start_entity("typedef", name=Util.ccolonName(self.scope(), typedef.name))
      self.write_entity("type", self.format_type(typedef.alias))
      self.end_entity("typedef")

   def visit_variable(self, variable):

      self.start_entity("fieldsynopsis")
      variable.vtype.accept(self)
      self.entity("type", self.type_label())
      self.entity("varname", variable.name[-1])
      self.end_entity("fieldsynopsis")

   def visit_const(self, const):

      print "sorry, <const> not implemented"

   def visit_module(self, module):

      self.start_entity("namespace", name=Util.ccolonName(self.scope(), module.name))
      self.write("\n")
      self.process_doc(module.annotations.get('doc', ''))
      self.push_scope(module.name)
      for declaration in module.declarations: declaration.accept(self)
      self.pop_scope()
      self.end_entity("namespace")

   def visit_class(self, class_):

      self.start_entity("class", name=Util.ccolonName(self.scope(), class_.name))
      # class_.type
      for p in class_.parents:
         p.accept(self)
      self.push_scope(class_.name)
      if class_.annotations.has_key('doc'):
         self.start_entity("purpose")
         self.process_doc(class_.annotations.get['doc'])
         self.end_entity("purpose")
      for d in class_.declarations:
         d.accept(self)
      self.pop_scope()
      self.end_entity("class")

   def visit_inheritance(self, inheritance):

      if len(inheritance.attributes()):
         self.start_entity("inherit", access=inheritance.attributes()[0])
      else:
         self.start_entity("inherit")
      self.write_entity("classname", self.format_type(inheritance.parent()))
      self.end_entity("inherit")

   def visit_parameter(self, parameter):

      self.start_entity("parameter", name=parameter.name)
      self.write_entity("param_type", self.format_type(parameter.type))
      #map(lambda m, this=self: this.write_entity("modifier", m), parameter.premodifier)
      #map(lambda m, this=self: this.write_entity("modifier", m), parameter.postmodifier)
      self.end_entity("parameter")
      self.write("\n")

   def visit_function(self, function):

      self.start_entity("function", name=Util.ccolonName(self.scope(), function.real_name))
      self.do_function(function)
      self.end_entity("function")
      self.write("\n")

   def visit_operation(self, operation):

      name = operation.name
      tag = None
      if len(name) > 1:
         if name[-1] == name[-2]:
            tag = "constructor"
            self.start_entity(tag)
         elif name[-1] == "~"+name[-2]:
            tag = "destructor"
            self.start_entity(tag)
      if tag is None:
         tag = "method"
         self.start_entity(tag, name=Util.ccolonName(self.scope(), operation.real_name))
      self.do_function(operation)
      self.end_entity(tag)
      self.write("\n")

   def do_function(self, func):
      """Stuff common to functions and methods, contructors, destructors"""

      for parameter in func.parameters: parameter.accept(self)
      if func.return_type:
         self.write_entity("type", self.format_type(func.return_type))
         self.write("\n")
      if func.annotations.has_key('doc'):
         self.start_entity("purpose")
         self.process_doc(func.annotations['doc'])
         self.end_entity("purpose")
         self.write("\n")
	
      if func.exceptions:
         self.start_entity("throws")
         for ex in func.exceptions:
            self.write_entity("simpara", ex)
         self.end_entity("throws")
         self.write("\n")

   def visit_enumerator(self, enumerator):

      print "sorry, <enumerator> not implemented"

   def visit_enum(self, enum):

      print "sorry, <enum> not implemented"

